#include "storage_engine.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

StorageEngine::StorageEngine(const std::string& root) : root_(root) {}

std::string StorageEngine::databasePath(const std::string& db) const {
    return root_ + "/" + db;
}

std::string StorageEngine::metaPath(const std::string& db, const std::string& table_name) const {
    return databasePath(db) + "/" + table_name + ".meta";
}

std::string StorageEngine::dataPath(const std::string& db, const std::string& table_name) const {
    return databasePath(db) + "/" + table_name + ".dat";
}

std::string StorageEngine::idxPath(const std::string& db, const std::string& table_name) const {
    return databasePath(db) + "/" + table_name + ".idx";
}

std::string StorageEngine::encodeCell(const std::string& v) {
    std::string r;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i] == '\\') r += "\\\\";
        else if (v[i] == '\t') r += "\\t";
        else if (v[i] == '\n') r += "\\n";
        else r += v[i];
    }
    return r;
}

std::string StorageEngine::decodeCell(const std::string& v) {
    std::string r;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i] == '\\' && i + 1 < v.size()) {
            char n = v[i + 1];
            if (n == 't') { r += '\t'; ++i; }
            else if (n == 'n') { r += '\n'; ++i; }
            else if (n == '\\') { r += '\\'; ++i; }
            else r += v[i];
        } else {
            r += v[i];
        }
    }
    return r;
}

bool StorageEngine::ensureRoot(std::string& err) const {
    try {
        std::filesystem::create_directories(root_);
        return true;
    } catch (const std::exception& e) {
        err = e.what();
        return false;
    }
}

bool StorageEngine::databaseExists(const std::string& db) const {
    return std::filesystem::exists(databasePath(db)) && std::filesystem::is_directory(databasePath(db));
}

bool StorageEngine::tableExists(const std::string& db, const std::string& table_name) const {
    return std::filesystem::exists(metaPath(db, table_name));
}

bool StorageEngine::createDatabase(const std::string& db, std::string& err) const {
    if (!ensureRoot(err)) return false;
    if (databaseExists(db)) {
        err = "database already exists: " + db;
        return false;
    }
    try {
        std::filesystem::create_directories(databasePath(db));
        return true;
    } catch (const std::exception& e) {
        err = e.what();
        return false;
    }
}

bool StorageEngine::dropDatabase(const std::string& db, std::string& err) const {
    if (!databaseExists(db)) {
        err = "database does not exist: " + db;
        return false;
    }
    try {
        std::filesystem::remove_all(databasePath(db));
        return true;
    } catch (const std::exception& e) {
        err = e.what();
        return false;
    }
}

bool StorageEngine::createTable(const std::string& db, const table& t, std::string& err) const {
    if (!databaseExists(db)) {
        err = "database does not exist: " + db;
        return false;
    }
    if (tableExists(db, t.name)) {
        err = "table already exists: " + t.name;
        return false;
    }
    return saveTable(db, t, err);
}

bool StorageEngine::dropTable(const std::string& db, const std::string& table_name, std::string& err) const {
    if (!tableExists(db, table_name)) {
        err = "table does not exist: " + table_name;
        return false;
    }
    try {
        std::filesystem::remove(metaPath(db, table_name));
        std::filesystem::remove(dataPath(db, table_name));
        std::filesystem::remove(idxPath(db, table_name));
        return true;
    } catch (const std::exception& e) {
        err = e.what();
        return false;
    }
}

bool StorageEngine::loadTable(const std::string& db, const std::string& table_name, table& t, std::string& err) const {
    if (!tableExists(db, table_name)) {
        err = "table does not exist: " + table_name;
        return false;
    }

    t = table();
    t.name = table_name;
    std::ifstream meta(metaPath(db, table_name));
    if (!meta) {
        err = "failed to open meta file";
        return false;
    }

    int col_count = 0;
    meta >> col_count;
    std::string dummy;
    std::getline(meta, dummy);
    for (int i = 0; i < col_count; ++i) {
        std::string name, type, primary_text;
        if (!(meta >> name >> type >> primary_text)) {
            err = "bad meta file format";
            return false;
        }
        ColumnType ct = (type == "int") ? ColumnType::Int : ColumnType::String;
        bool primary = (primary_text == "1");
        t.columns.push_back(column(name, ct, primary));
        if (primary) t.primary_index = i;
    }

    std::ifstream dat(dataPath(db, table_name));
    if (!dat) {
        err = "failed to open data file";
        return false;
    }
    std::string line;
    while (std::getline(dat, line)) {
        if (line.empty()) continue;
        row r;
        std::size_t start = 0;
        while (true) {
            std::size_t tab = line.find('\t', start);
            if (tab == std::string::npos) {
                r.values.push_back(decodeCell(line.substr(start)));
                break;
            }
            r.values.push_back(decodeCell(line.substr(start, tab - start)));
            start = tab + 1;
        }
        if (r.values.size() != t.columns.size()) {
            err = "bad data file format: column count mismatch";
            return false;
        }
        t.rows.push_back(r);
    }
    return true;
}

bool StorageEngine::saveTable(const std::string& db, const table& t, std::string& err) const {
    try {
        std::filesystem::create_directories(databasePath(db));
    } catch (const std::exception& e) {
        err = e.what();
        return false;
    }

    std::ofstream meta(metaPath(db, t.name));
    if (!meta) {
        err = "failed to write meta file";
        return false;
    }
    meta << t.columns.size() << '\n';
    for (std::size_t i = 0; i < t.columns.size(); ++i) {
        meta << t.columns[i].name << ' '
             << (t.columns[i].type == ColumnType::Int ? "int" : "string") << ' '
             << (t.columns[i].primary ? "1" : "0") << '\n';
    }

    std::ofstream dat(dataPath(db, t.name));
    if (!dat) {
        err = "failed to write data file";
        return false;
    }
    for (std::size_t r = 0; r < t.rows.size(); ++r) {
        for (std::size_t c = 0; c < t.rows[r].values.size(); ++c) {
            if (c > 0) dat << '\t';
            dat << encodeCell(t.rows[r].values[c]);
        }
        dat << '\n';
    }

    if (t.primary_index >= 0) {
        index idx(t.columns[t.primary_index].type);
        for (std::size_t r = 0; r < t.rows.size(); ++r) {
            idx.insert(t.rows[r].values[t.primary_index], static_cast<int>(r));
        }
        idx.dumpToFile(idxPath(db, t.name));
    } else {
        std::filesystem::remove(idxPath(db, t.name));
    }
    return true;
}
