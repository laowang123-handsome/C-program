#ifndef MINISQL_RESULT_CODEC_HPP
#define MINISQL_RESULT_CODEC_HPP

#include <string>
#include "schema.hpp"

class ResultCodec {
public:
    static std::string serialize(const QueryResult& result);
    static bool deserialize(const std::string& text, QueryResult& result, std::string& err);
};

#endif
