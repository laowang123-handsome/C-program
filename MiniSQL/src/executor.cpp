#include "executor.hpp"
#include <filesystem>
#include <cstdlib>

static bool isIntegerLiteral(const std::string& s) {
    if (s.empty()) return false;
    std::size_t start = (s[0] == '-') ? 1 : 0;
    if (start >= s.size()) return false;
    for (std::size_t i = start; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

database::database(const std::string& root) : storage_(root), current_db_() {
    std::string err;
    storage_.ensureRoot(err);
}

QueryResult database::error(const std::string& msg) const {
    QueryResult r;
    r.ok = false;
    r.message = "ERROR: " + msg;
    return r;
}

bool database::requireCurrentDb(QueryResult& result) const {
    if (current_db_.empty()) {
        result = error("no database selected; use <dbname> first");
        return false;
    }
    return true;
}

bool database::validateValue(ColumnType type, const std::string& value, std::string& err) const {
    if (type == ColumnType::Int) {
        if (!isIntegerLiteral(value)) {
            err = "invalid int value: " + value;
            return false;
        }
        return true;
    }
    if (value.size() > 256) {
        err = "string value is longer than 256 characters";
        return false;
    }
    return true;
}

int database::compareValue(ColumnType type, const std::string& a, const std::string& b) const {
    if (type == ColumnType::Int) {
        int ai = std::stoi(a);
        int bi = std::stoi(b);
        if (ai < bi) return -1;
        if (ai > bi) return 1;
        return 0;
    }
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

bool database::rowMatches(const table& t, const row& r, const Condition& cond, std::string& err) const {
    if (!cond.has) return true;
    int ci = t.columnIndex(cond.column);
    if (ci < 0) {
        err = "unknown column in where: " + cond.column;
        return false;
    }
    if (!validateValue(t.columns[ci].type, cond.value, err)) return false;
    int cmp = compareValue(t.columns[ci].type, r.values[ci], cond.value);
    if (cond.op == "=") return cmp == 0;
    if (cond.op == "<") return cmp < 0;
    if (cond.op == ">") return cmp > 0;
    err = "unsupported operator: " + cond.op;
    return false;
}

bool database::buildIndex(const table& t, index& idx, std::string& err) const {
    if (t.primary_index < 0) return true;
    for (std::size_t i = 0; i < t.rows.size(); ++i) {
        std::string key = t.rows[i].values[t.primary_index];
        if (idx.findEqual(key) >= 0) {
            err = "duplicate primary key: " + key;
            return false;
        }
        idx.insert(key, static_cast<int>(i));
    }
    return true;
}

void database::collectCandidateRows(const table& t, const Condition& cond, index& idx,
                                    SimpleArray<int>& candidates, bool& used_index) const {
    used_index = false;
    if (cond.has && t.primary_index >= 0 && cond.column == t.columns[t.primary_index].name) {
        used_index = true;
        if (cond.op == "=") {
            int r = idx.findEqual(cond.value);
            if (r >= 0) candidates.push_back(r);
        } else if (cond.op == "<") {
            idx.findLess(cond.value, candidates);
        } else if (cond.op == ">") {
            idx.findGreater(cond.value, candidates);
        }
        return;
    }
    for (std::size_t i = 0; i < t.rows.size(); ++i) {
        candidates.push_back(static_cast<int>(i));
    }
}

QueryResult database::execute(const std::string& sql) {
    SqlStatement stmt;
    std::string err;
    if (!SqlParser::parse(sql, stmt, err)) {
        return error(err);
    }

    QueryResult result;
    switch (stmt.type) {
        case SqlType::CreateDatabase:
            if (!storage_.createDatabase(stmt.db_name, err)) return error(err);
            result.message = "Query OK, database created";
            return result;
        case SqlType::DropDatabase:
            if (!storage_.dropDatabase(stmt.db_name, err)) return error(err);
            if (current_db_ == stmt.db_name) current_db_.clear();
            result.message = "Query OK, database dropped";
            return result;
        case SqlType::UseDatabase:
            if (!storage_.databaseExists(stmt.db_name)) return error("database does not exist: " + stmt.db_name);
            current_db_ = stmt.db_name;
            result.message = "Database changed";
            return result;
        case SqlType::CreateTable:
            return executeCreateTable(stmt);
        case SqlType::DropTable:
            if (!requireCurrentDb(result)) return result;
            if (!storage_.dropTable(current_db_, stmt.table_name, err)) return error(err);
            result.message = "Query OK, table dropped";
            return result;
        case SqlType::Insert:
            return executeInsert(stmt);
        case SqlType::Select:
            return executeSelect(stmt);
        case SqlType::DeleteRows:
            return executeDelete(stmt);
        case SqlType::UpdateRows:
            return executeUpdate(stmt);
        case SqlType::ExitCommand:
            result.message = "Bye";
            return result;
        default:
            return error("unsupported SQL statement");
    }
}

QueryResult database::executeCreateTable(const SqlStatement& stmt) {
    QueryResult result;
    if (!requireCurrentDb(result)) return result;
    table t;
    t.name = stmt.table_name;
    bool primary_seen = false;
    for (std::size_t i = 0; i < stmt.create_columns.size(); ++i) {
        for (std::size_t j = 0; j < t.columns.size(); ++j) {
            if (t.columns[j].name == stmt.create_columns[i].name) {
                return error("duplicate column name: " + stmt.create_columns[i].name);
            }
        }
        column c(stmt.create_columns[i].name, stmt.create_columns[i].type, stmt.create_columns[i].primary);
        if (c.primary) {
            if (primary_seen) return error("only one primary column is supported");
            primary_seen = true;
            t.primary_index = static_cast<int>(i);
        }
        t.columns.push_back(c);
    }
    if (t.columns.empty()) return error("table must have at least one column");

    std::string err;
    if (!storage_.createTable(current_db_, t, err)) return error(err);
    result.message = "Query OK, table created";
    return result;
}

QueryResult database::executeInsert(const SqlStatement& stmt) {
    QueryResult result;
    if (!requireCurrentDb(result)) return result;
    table t;
    std::string err;
    if (!storage_.loadTable(current_db_, stmt.table_name, t, err)) return error(err);
    if (stmt.values.size() != t.columns.size()) {
        return error("insert value count does not match column count");
    }

    row r;
    for (std::size_t i = 0; i < stmt.values.size(); ++i) {
        if (!validateValue(t.columns[i].type, stmt.values[i], err)) return error(err);
        r.values.push_back(stmt.values[i]);
    }

    if (t.primary_index >= 0) {
        index idx(t.columns[t.primary_index].type);
        if (!buildIndex(t, idx, err)) return error(err);
        std::string key = r.values[t.primary_index];
        if (idx.findEqual(key) >= 0) {
            return error("duplicate primary key: " + key);
        }
    }

    t.rows.push_back(r);
    if (!storage_.saveTable(current_db_, t, err)) return error(err);
    result.affected = 1;
    result.message = "Query OK, 1 row affected";
    return result;
}

QueryResult database::executeSelect(const SqlStatement& stmt) {
    QueryResult result;
    if (!requireCurrentDb(result)) return result;
    table t;
    std::string err;
    if (!storage_.loadTable(current_db_, stmt.table_name, t, err)) return error(err);

    int selected_index = -1;
    bool all_columns = (stmt.select_column == "*");
    if (!all_columns) {
        selected_index = t.columnIndex(stmt.select_column);
        if (selected_index < 0) return error("unknown selected column: " + stmt.select_column);
        result.headers.push_back(stmt.select_column);
    } else {
        for (std::size_t i = 0; i < t.columns.size(); ++i) {
            result.headers.push_back(t.columns[i].name);
        }
    }

    if (stmt.condition.has) {
        int ci = t.columnIndex(stmt.condition.column);
        if (ci < 0) return error("unknown column in where: " + stmt.condition.column);
        if (!validateValue(t.columns[ci].type, stmt.condition.value, err)) return error(err);
    }

    index idx(t.primary_index >= 0 ? t.columns[t.primary_index].type : ColumnType::Int);
    if (t.primary_index >= 0 && !buildIndex(t, idx, err)) return error(err);
    SimpleArray<int> candidates;
    bool used_index = false;
    collectCandidateRows(t, stmt.condition, idx, candidates, used_index);

    for (std::size_t i = 0; i < candidates.size(); ++i) {
        int ri = candidates[i];
        if (ri < 0 || static_cast<std::size_t>(ri) >= t.rows.size()) continue;
        bool ok = rowMatches(t, t.rows[ri], stmt.condition, err);
        if (!err.empty()) return error(err);
        if (!ok) continue;
        row out;
        if (all_columns) {
            for (std::size_t c = 0; c < t.rows[ri].values.size(); ++c) {
                out.values.push_back(t.rows[ri].values[c]);
            }
        } else {
            out.values.push_back(t.rows[ri].values[selected_index]);
        }
        result.rows.push_back(out);
    }

    result.affected = static_cast<int>(result.rows.size());
    result.message = used_index ? "Query OK, index used" : "Query OK";
    return result;
}

QueryResult database::executeDelete(const SqlStatement& stmt) {
    QueryResult result;
    if (!requireCurrentDb(result)) return result;
    table t;
    std::string err;
    if (!storage_.loadTable(current_db_, stmt.table_name, t, err)) return error(err);

    if (stmt.condition.has) {
        int ci = t.columnIndex(stmt.condition.column);
        if (ci < 0) return error("unknown column in where: " + stmt.condition.column);
        if (!validateValue(t.columns[ci].type, stmt.condition.value, err)) return error(err);
    }

    table nt;
    nt.name = t.name;
    nt.primary_index = t.primary_index;
    for (std::size_t c = 0; c < t.columns.size(); ++c) nt.columns.push_back(t.columns[c]);

    int deleted = 0;
    for (std::size_t i = 0; i < t.rows.size(); ++i) {
        bool match = rowMatches(t, t.rows[i], stmt.condition, err);
        if (!err.empty()) return error(err);
        if (match) {
            ++deleted;
        } else {
            nt.rows.push_back(t.rows[i]);
        }
    }

    if (!storage_.saveTable(current_db_, nt, err)) return error(err);
    result.affected = deleted;
    result.message = "Query OK, " + std::to_string(deleted) + " row(s) affected";
    return result;
}

QueryResult database::executeUpdate(const SqlStatement& stmt) {
    QueryResult result;
    if (!requireCurrentDb(result)) return result;
    table t;
    std::string err;
    if (!storage_.loadTable(current_db_, stmt.table_name, t, err)) return error(err);

    int set_index = t.columnIndex(stmt.set_column);
    if (set_index < 0) return error("unknown column in set: " + stmt.set_column);
    if (!validateValue(t.columns[set_index].type, stmt.set_value, err)) return error(err);
    if (stmt.condition.has) {
        int ci = t.columnIndex(stmt.condition.column);
        if (ci < 0) return error("unknown column in where: " + stmt.condition.column);
        if (!validateValue(t.columns[ci].type, stmt.condition.value, err)) return error(err);
    }

    int updated = 0;
    for (std::size_t i = 0; i < t.rows.size(); ++i) {
        bool match = rowMatches(t, t.rows[i], stmt.condition, err);
        if (!err.empty()) return error(err);
        if (match) {
            t.rows[i].values[set_index] = stmt.set_value;
            ++updated;
        }
    }

    if (t.primary_index >= 0) {
        index idx(t.columns[t.primary_index].type);
        if (!buildIndex(t, idx, err)) return error(err);
    }

    if (!storage_.saveTable(current_db_, t, err)) return error(err);
    result.affected = updated;
    result.message = "Query OK, " + std::to_string(updated) + " row(s) affected";
    return result;
}
