#include "field_filter.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace log_query {

FieldEqualFilter::FieldEqualFilter(std::string field, std::string value)
    : field_(std::move(field))
    , value_(std::move(value))
{
}

bool FieldEqualFilter::match(const LogLine& line) const {
    auto it = line.fields.find(field_);
    if (it == line.fields.end()) return false;

    if (std::holds_alternative<int64_t>(it->second)) {
        try {
            size_t pos = 0;
            int64_t expected = std::stoll(value_, &pos);
            if (pos != value_.size()) return false;
            return std::get<int64_t>(it->second) == expected;
        } catch (...) {
            return false;
        }
    }

    if (std::holds_alternative<double>(it->second)) {
        try {
            size_t pos = 0;
            double expected = std::stod(value_, &pos);
            if (pos != value_.size()) return false;
            double actual = std::get<double>(it->second);
            return std::abs(actual - expected) < 1e-9;
        } catch (...) {
            return false;
        }
    }

    if (std::holds_alternative<std::string>(it->second)) {
        std::string field_val = std::get<std::string>(it->second);
        std::string expected = value_;

        std::transform(field_val.begin(), field_val.end(), field_val.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        std::transform(expected.begin(), expected.end(), expected.begin(),
                       [](unsigned char c) { return std::toupper(c); });

        return field_val == expected;
    }

    return false;
}

std::string FieldEqualFilter::describe() const {
    return field_ + " == " + value_;
}

} // namespace log_query
