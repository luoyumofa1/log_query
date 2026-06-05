#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <variant>

namespace log_query {

using FieldValue = std::variant<
    std::string,
    int64_t,
    double,
    std::chrono::system_clock::time_point
>;

struct LogLine {
    int64_t line_number = 0;
    std::string raw_text;
    std::map<std::string, FieldValue> fields;
    std::chrono::system_clock::time_point timestamp;
};

} // namespace log_query
