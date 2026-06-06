#include "compare_filter.h"

#include <cmath>
#include <sstream>

namespace log_query {

CompareFilter::CompareFilter(std::string field, Op op, double threshold)
    : field_(std::move(field))
    , op_(op)
    , threshold_(threshold)
{
}

double CompareFilter::to_double(const FieldValue& value) {
    if (std::holds_alternative<int64_t>(value)) {
        return static_cast<double>(std::get<int64_t>(value));
    }
    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }
    if (std::holds_alternative<std::string>(value)) {
        try {
            size_t pos = 0;
            double d = std::stod(std::get<std::string>(value), &pos);
            if (pos == std::get<std::string>(value).size()) return d;
        } catch (...) {}
    }
    return std::nan("");
}

bool CompareFilter::match(const LogLine& line) const {
    auto it = line.fields.find(field_);
    if (it == line.fields.end()) return false;

    double val = to_double(it->second);
    if (std::isnan(val)) return false;

    switch (op_) {
    case Op::Greater:      return val > threshold_;
    case Op::GreaterEqual: return val >= threshold_;
    case Op::Less:         return val < threshold_;
    case Op::LessEqual:    return val <= threshold_;
    case Op::NotEqual:     return val != threshold_;
    }
    return false;
}

std::string CompareFilter::describe() const {
    std::ostringstream oss;
    oss << field_;
    switch (op_) {
    case Op::Greater:      oss << " > "; break;
    case Op::GreaterEqual: oss << " >= "; break;
    case Op::Less:         oss << " < "; break;
    case Op::LessEqual:    oss << " <= "; break;
    case Op::NotEqual:     oss << " != "; break;
    }
    oss << threshold_;
    return oss.str();
}

} // namespace log_query
