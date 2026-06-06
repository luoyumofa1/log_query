#include "log_format.h"

#include <fstream>
#include <json.hpp>
#include <stdexcept>

namespace log_query {

static FieldType parse_field_type(const std::string& type_str) {
    if (type_str == "string")   return FieldType::String;
    if (type_str == "int")      return FieldType::Int;
    if (type_str == "float")    return FieldType::Float;
    if (type_str == "datetime") return FieldType::DateTime;
    if (type_str == "enum")     return FieldType::Enum;
    if (type_str == "regex")    return FieldType::Regex;
    throw std::runtime_error("Unknown field type: " + type_str);
}

LogFormatConfig load_format_config(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open format config: " + path);
    }

    nlohmann::json j;
    file >> j;

    LogFormatConfig config;
    config.name = j.at("name").get<std::string>();
    config.pattern = j.at("pattern").get<std::string>();
    config.level_field = j.value("level_field", "");
    config.module_field = j.value("module_field", "");

    for (auto& [field_name, field_json] : j.at("fields").items()) {
        FieldConfig fc;
        fc.type = parse_field_type(field_json.at("type").get<std::string>());

        if (field_json.contains("format")) {
            fc.datetime_format = field_json.at("format").get<std::string>();
        }
        if (field_json.contains("values")) {
            fc.enum_values = field_json.at("values").get<std::vector<std::string>>();
        }
        if (field_json.contains("greedy")) {
            fc.greedy = field_json.at("greedy").get<bool>();
        }
        if (field_json.contains("pattern")) {
            fc.regex_pattern = field_json.at("pattern").get<std::string>();
        }

        config.fields[field_name] = std::move(fc);
    }

    return config;
}

} // namespace log_query
