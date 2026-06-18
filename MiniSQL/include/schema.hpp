#ifndef MINISQL_SCHEMA_HPP
#define MINISQL_SCHEMA_HPP

#include <string>
#include "simple_array.hpp"

enum class ColumnType {
    Int,
    String
};

struct column {
    std::string name;
    ColumnType type;
    bool primary;

    column() : name(), type(ColumnType::String), primary(false) {}
    column(const std::string& n, ColumnType t, bool p)
        : name(n), type(t), primary(p) {}
};

struct row {
    SimpleArray<std::string> values;
};

struct Condition {
    bool has;
    std::string column;
    std::string op;
    std::string value;

    Condition() : has(false), column(), op(), value() {}
};

struct table {
    std::string name;
    SimpleArray<column> columns;
    SimpleArray<row> rows;
    int primary_index;

    table() : name(), columns(), rows(), primary_index(-1) {}

    int columnIndex(const std::string& col_name) const {
        for (std::size_t i = 0; i < columns.size(); ++i) {
            if (columns[i].name == col_name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
};

struct QueryResult {
    bool ok;
    std::string message;
    int affected;
    SimpleArray<std::string> headers;
    SimpleArray<row> rows;

    QueryResult() : ok(true), message("OK"), affected(0), headers(), rows() {}
};

#endif
