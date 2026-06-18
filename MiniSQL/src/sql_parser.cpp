#include "sql_parser.hpp"
#include <cctype>

std::string SqlParser::lowerKeyword(const std::string& s) {
    std::string r = s;
    for (std::size_t i = 0; i < r.size(); ++i) {
        r[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(r[i])));
    }
    return r;
}

bool SqlParser::isName(const std::string& s) {
    if (s.empty()) return false;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] < 'a' || s[i] > 'z') {
            return false;
        }
    }
    return true;
}

bool SqlParser::tokenize(const std::string& sql, SimpleArray<std::string>& tokens, std::string& err) {
    std::size_t i = 0;
    while (i < sql.size()) {
        char c = sql[i];
        if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
            continue;
        }
        if (c == ';') {
            ++i;
            continue;
        }
        if (c == '(' || c == ')' || c == ',' || c == '=' || c == '<' || c == '>') {
            std::string t;
            t += c;
            tokens.push_back(t);
            ++i;
            continue;
        }
        if (c == '"') {
            ++i;
            std::string value;
            bool closed = false;
            while (i < sql.size()) {
                if (sql[i] == '"') {
                    closed = true;
                    ++i;
                    break;
                }
                value += sql[i];
                ++i;
            }
            if (!closed) {
                err = "unclosed string literal";
                return false;
            }
            tokens.push_back(value);
            continue;
        }
        std::string t;
        while (i < sql.size()) {
            char d = sql[i];
            if (std::isspace(static_cast<unsigned char>(d)) || d == '(' || d == ')' ||
                d == ',' || d == '=' || d == '<' || d == '>' || d == ';') {
                break;
            }
            t += d;
            ++i;
        }
        tokens.push_back(t);
    }
    return true;
}

bool SqlParser::parseCondition(const SimpleArray<std::string>& tokens, std::size_t start,
                               Condition& cond, std::string& err) {
    if (start + 2 >= tokens.size()) {
        err = "invalid where condition";
        return false;
    }
    cond.has = true;
    cond.column = tokens[start];
    cond.op = tokens[start + 1];
    cond.value = tokens[start + 2];
    if (!isName(cond.column)) {
        err = "invalid column name in where condition";
        return false;
    }
    if (!(cond.op == "=" || cond.op == "<" || cond.op == ">")) {
        err = "unsupported where operator";
        return false;
    }
    if (start + 3 != tokens.size()) {
        err = "unexpected tokens after where condition";
        return false;
    }
    return true;
}

bool SqlParser::parse(const std::string& sql, SqlStatement& stmt, std::string& err) {
    SimpleArray<std::string> tokens;
    if (!tokenize(sql, tokens, err)) return false;
    if (tokens.empty()) {
        err = "empty SQL";
        return false;
    }

    std::string first = lowerKeyword(tokens[0]);
    if (first == "exit" || first == "quit") {
        stmt.type = SqlType::ExitCommand;
        return true;
    }

    if (first == "create") {
        if (tokens.size() < 3) { err = "invalid create statement"; return false; }
        std::string second = lowerKeyword(tokens[1]);
        if (second == "database") {
            if (tokens.size() != 3 || !isName(tokens[2])) { err = "invalid create database syntax"; return false; }
            stmt.type = SqlType::CreateDatabase;
            stmt.db_name = tokens[2];
            return true;
        }
        if (second == "table") {
            if (tokens.size() < 7 || !isName(tokens[2]) || tokens[3] != "(") {
                err = "invalid create table syntax";
                return false;
            }
            stmt.type = SqlType::CreateTable;
            stmt.table_name = tokens[2];
            std::size_t pos = 4;
            bool primary_seen = false;
            while (pos < tokens.size()) {
                if (tokens[pos] == ")") {
                    if (pos + 1 != tokens.size()) { err = "unexpected tokens after create table"; return false; }
                    return true;
                }
                if (pos + 1 >= tokens.size()) { err = "invalid column definition"; return false; }
                ParsedColumn pc;
                pc.name = tokens[pos];
                if (!isName(pc.name)) { err = "invalid column name"; return false; }
                std::string type_token = lowerKeyword(tokens[pos + 1]);
                if (type_token == "int") pc.type = ColumnType::Int;
                else if (type_token == "string") pc.type = ColumnType::String;
                else { err = "unsupported column type"; return false; }
                pc.primary = false;
                pos += 2;
                if (pos < tokens.size() && lowerKeyword(tokens[pos]) == "primary") {
                    if (primary_seen) { err = "only one primary column is supported"; return false; }
                    pc.primary = true;
                    primary_seen = true;
                    ++pos;
                }
                stmt.create_columns.push_back(pc);
                if (pos < tokens.size() && tokens[pos] == ",") {
                    ++pos;
                    continue;
                }
                if (pos < tokens.size() && tokens[pos] == ")") {
                    continue;
                }
                err = "expected comma or right parenthesis in create table";
                return false;
            }
            err = "missing right parenthesis in create table";
            return false;
        }
    }

    if (first == "drop") {
        if (tokens.size() != 3) { err = "invalid drop statement"; return false; }
        std::string second = lowerKeyword(tokens[1]);
        if (second == "database") {
            if (!isName(tokens[2])) { err = "invalid database name"; return false; }
            stmt.type = SqlType::DropDatabase;
            stmt.db_name = tokens[2];
            return true;
        }
        if (second == "table") {
            if (!isName(tokens[2])) { err = "invalid table name"; return false; }
            stmt.type = SqlType::DropTable;
            stmt.table_name = tokens[2];
            return true;
        }
    }

    if (first == "use") {
        if (tokens.size() != 2 || !isName(tokens[1])) { err = "invalid use syntax"; return false; }
        stmt.type = SqlType::UseDatabase;
        stmt.db_name = tokens[1];
        return true;
    }

    if (first == "insert") {
        if (tokens.size() < 6 || !isName(tokens[1]) || lowerKeyword(tokens[2]) != "values" || tokens[3] != "(") {
            err = "invalid insert syntax";
            return false;
        }
        stmt.type = SqlType::Insert;
        stmt.table_name = tokens[1];
        std::size_t pos = 4;
        while (pos < tokens.size()) {
            if (tokens[pos] == ")") {
                if (pos + 1 != tokens.size()) { err = "unexpected tokens after insert"; return false; }
                return true;
            }
            stmt.values.push_back(tokens[pos]);
            ++pos;
            if (pos < tokens.size() && tokens[pos] == ",") {
                ++pos;
                continue;
            }
            if (pos < tokens.size() && tokens[pos] == ")") {
                continue;
            }
            err = "expected comma or right parenthesis in insert";
            return false;
        }
        err = "missing right parenthesis in insert";
        return false;
    }

    if (first == "select") {
        if (tokens.size() < 4) { err = "invalid select syntax"; return false; }
        stmt.type = SqlType::Select;
        stmt.select_column = tokens[1];
        if (!(stmt.select_column == "*" || isName(stmt.select_column))) { err = "invalid selected column"; return false; }
        if (lowerKeyword(tokens[2]) != "from" || !isName(tokens[3])) { err = "invalid select from syntax"; return false; }
        stmt.table_name = tokens[3];
        if (tokens.size() == 4) return true;
        if (tokens.size() >= 8 && lowerKeyword(tokens[4]) == "where") {
            return parseCondition(tokens, 5, stmt.condition, err);
        }
        err = "invalid select where syntax";
        return false;
    }

    if (first == "delete") {
        if (tokens.size() < 2 || !isName(tokens[1])) { err = "invalid delete syntax"; return false; }
        stmt.type = SqlType::DeleteRows;
        stmt.table_name = tokens[1];
        if (tokens.size() == 2) return true;
        if (tokens.size() >= 6 && lowerKeyword(tokens[2]) == "where") {
            return parseCondition(tokens, 3, stmt.condition, err);
        }
        err = "invalid delete where syntax";
        return false;
    }

    if (first == "update") {
        if (tokens.size() < 6 || !isName(tokens[1]) || lowerKeyword(tokens[2]) != "set" ||
            !isName(tokens[3]) || tokens[4] != "=") {
            err = "invalid update syntax";
            return false;
        }
        stmt.type = SqlType::UpdateRows;
        stmt.table_name = tokens[1];
        stmt.set_column = tokens[3];
        stmt.set_value = tokens[5];
        if (tokens.size() == 6) return true;
        if (tokens.size() >= 10 && lowerKeyword(tokens[6]) == "where") {
            return parseCondition(tokens, 7, stmt.condition, err);
        }
        err = "invalid update where syntax";
        return false;
    }

    err = "unsupported SQL statement";
    return false;
}
