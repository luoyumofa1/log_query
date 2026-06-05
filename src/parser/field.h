#pragma once

#include "config/log_format.h"
#include "log_query/log_line.h"

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace log_query {

std::optional<FieldValue> convert_field(
    std::string_view raw,
    FieldType type,
    const std::string& datetime_format,
    const std::vector<std::string>& enum_values);

int64_t parse_timestamp(std::string_view raw, const std::string& format);

int compare_enum(const std::string& value, const std::vector<std::string>& values);

} // namespace log_query
