#ifndef MINISQL_STORAGE_ENGINE_HPP
#define MINISQL_STORAGE_ENGINE_HPP

#include <string>
#include "schema.hpp"
#include "bplustree.hpp"

class StorageEngine {
private:
    std::string root_;

    std::string databasePath(const std::string& db) const;
    std::string metaPath(const std::string& db, const std::string& table_name) const;
    std::string dataPath(const std::string& db, const std::string& table_name) const;
    std::string idxPath(const std::string& db, const std::string& table_name) const;

    static std::string encodeCell(const std::string& v);
    static std::string decodeCell(const std::string& v);

public:
    explicit StorageEngine(const std::string& root = "data");

    bool ensureRoot(std::string& err) const;
    bool databaseExists(const std::string& db) const;
    bool tableExists(const std::string& db, const std::string& table_name) const;

    bool createDatabase(const std::string& db, std::string& err) const;
    bool dropDatabase(const std::string& db, std::string& err) const;
    bool createTable(const std::string& db, const table& t, std::string& err) const;
    bool dropTable(const std::string& db, const std::string& table_name, std::string& err) const;
    bool loadTable(const std::string& db, const std::string& table_name, table& t, std::string& err) const;
    bool saveTable(const std::string& db, const table& t, std::string& err) const;
};

#endif
