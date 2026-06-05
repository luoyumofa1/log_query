#include "field_filter.h"

#include <algorithm>
#include <cctype>

namespace log_query {

FieldEqualFilter::FieldEqualFilter(std::string field, std::string value)
    : field_(std::move(field))
    , value_(std::move(value))
{
}

bool FieldEqualFilter::match(const LogLine& line) const {
    auto it = line.fields.find(field_);
    if (it == line.fields.end()) return false;

    if (!std::holds_alternative<std::string>(it->second)) return false;

    std::string field_val = std::get<std::string>(it->second);
    std::string expected = value_;

    std::transform(field_val.begin(), field_val.end(), field_val.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    std::transform(expected.begin(), expected.end(), expected.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    return field_val == expected;
}

std::string FieldEqualFilter::describe() const {
    return field_ + " == " + value_;
}

} // namespace log_query
