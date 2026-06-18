#include "result_codec.hpp"
#include <sstream>

static std::string escapeLine(const std::string& s) {
    std::string r;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\') r += "\\\\";
        else if (s[i] == '\n') r += "\\n";
        else if (s[i] == '\t') r += "\\t";
        else r += s[i];
    }
    return r;
}

static std::string unescapeLine(const std::string& s) {
    std::string r;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[i + 1];
            if (n == 'n') { r += '\n'; ++i; }
            else if (n == 't') { r += '\t'; ++i; }
            else if (n == '\\') { r += '\\'; ++i; }
            else r += s[i];
        } else {
            r += s[i];
        }
    }
    return r;
}

std::string ResultCodec::serialize(const QueryResult& result) {
    std::string text;
    text += result.ok ? "OK\n" : "ERR\n";
    text += escapeLine(result.message) + "\n";
    text += std::to_string(result.affected) + "\n";
    text += std::to_string(result.headers.size()) + "\n";
    for (std::size_t i = 0; i < result.headers.size(); ++i) {
        text += escapeLine(result.headers[i]) + "\n";
    }
    text += std::to_string(result.rows.size()) + "\n";
    for (std::size_t r = 0; r < result.rows.size(); ++r) {
        text += std::to_string(result.rows[r].values.size()) + "\n";
        for (std::size_t c = 0; c < result.rows[r].values.size(); ++c) {
            text += escapeLine(result.rows[r].values[c]) + "\n";
        }
    }
    text += "END\n";
    return text;
}

bool ResultCodec::deserialize(const std::string& text, QueryResult& result, std::string& err) {
    std::istringstream in(text);
    std::string line;
    if (!std::getline(in, line)) { err = "missing status"; return false; }
    result.ok = (line == "OK");
    if (!(line == "OK" || line == "ERR")) { err = "bad status"; return false; }

    if (!std::getline(in, line)) { err = "missing message"; return false; }
    result.message = unescapeLine(line);

    if (!std::getline(in, line)) { err = "missing affected"; return false; }
    result.affected = std::stoi(line);

    if (!std::getline(in, line)) { err = "missing header count"; return false; }
    int header_count = std::stoi(line);
    for (int i = 0; i < header_count; ++i) {
        if (!std::getline(in, line)) { err = "missing header"; return false; }
        result.headers.push_back(unescapeLine(line));
    }

    if (!std::getline(in, line)) { err = "missing row count"; return false; }
    int row_count = std::stoi(line);
    for (int r = 0; r < row_count; ++r) {
        if (!std::getline(in, line)) { err = "missing cell count"; return false; }
        int cell_count = std::stoi(line);
        row rr;
        for (int c = 0; c < cell_count; ++c) {
            if (!std::getline(in, line)) { err = "missing cell"; return false; }
            rr.values.push_back(unescapeLine(line));
        }
        result.rows.push_back(rr);
    }
    if (!std::getline(in, line) || line != "END") {
        err = "missing END marker";
        return false;
    }
    return true;
}
