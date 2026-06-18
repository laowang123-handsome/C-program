#ifndef MINISQL_SQL_PARSER_HPP
#define MINISQL_SQL_PARSER_HPP

#include <string>
#include "simple_array.hpp"
#include "schema.hpp"

enum class SqlType {
    Unknown,
    CreateDatabase,
    DropDatabase,
    UseDatabase,
    CreateTable,
    DropTable,
    Insert,
    Select,
    DeleteRows,
    UpdateRows,
    ExitCommand
};

struct ParsedColumn {
    std::string name;
    ColumnType type;
    bool primary;

    ParsedColumn() : name(), type(ColumnType::String), primary(false) {}
};

struct SqlStatement {
    SqlType type;
    std::string db_name;
    std::string table_name;
    std::string select_column;
    SimpleArray<ParsedColumn> create_columns;
    SimpleArray<std::string> values;
    std::string set_column;
    std::string set_value;
    Condition condition;

    SqlStatement() : type(SqlType::Unknown), db_name(), table_name(), select_column("*"),
                     create_columns(), values(), set_column(), set_value(), condition() {}
};

class SqlParser {
private:
    static std::string lowerKeyword(const std::string& s);
    static bool tokenize(const std::string& sql, SimpleArray<std::string>& tokens, std::string& err);
    static bool isName(const std::string& s);
    static bool parseCondition(const SimpleArray<std::string>& tokens, std::size_t start,
                               Condition& cond, std::string& err);

public:
    static bool parse(const std::string& sql, SqlStatement& stmt, std::string& err);
};

#endif
