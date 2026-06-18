#ifndef MINISQL_EXECUTOR_HPP
#define MINISQL_EXECUTOR_HPP

#include <string>
#include "schema.hpp"
#include "sql_parser.hpp"
#include "storage_engine.hpp"
#include "bplustree.hpp"

class database {
private:
    StorageEngine storage_;
    std::string current_db_;

    QueryResult error(const std::string& msg) const;
    bool requireCurrentDb(QueryResult& result) const;
    bool validateValue(ColumnType type, const std::string& value, std::string& err) const;
    int compareValue(ColumnType type, const std::string& a, const std::string& b) const;
    bool rowMatches(const table& t, const row& r, const Condition& cond, std::string& err) const;
    bool buildIndex(const table& t, index& idx, std::string& err) const;
    void collectCandidateRows(const table& t, const Condition& cond, index& idx,
                              SimpleArray<int>& candidates, bool& used_index) const;

    QueryResult executeCreateTable(const SqlStatement& stmt);
    QueryResult executeInsert(const SqlStatement& stmt);
    QueryResult executeSelect(const SqlStatement& stmt);
    QueryResult executeDelete(const SqlStatement& stmt);
    QueryResult executeUpdate(const SqlStatement& stmt);

public:
    explicit database(const std::string& root = "data");
    QueryResult execute(const std::string& sql);
    const std::string& currentDatabase() const { return current_db_; }
};

#endif
