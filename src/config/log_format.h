#pragma once

#include <map>
#include <string>
#include <vector>

namespace log_query {

enum class FieldType {
    String,
    Int,
    Float,
    DateTime,
    Enum,
    Regex
};

struct FieldConfig {
    FieldType type = FieldType::String;
    std::string datetime_format;
    std::vector<std::string> enum_values;
    bool greedy = false;
    std::string regex_pattern;
};

struct LogFormatConfig {
    std::string name;
    std::string pattern;
    std::map<std::string, FieldConfig> fields;
};

LogFormatConfig load_format_config(const std::string& path);

} // namespace log_query
