#pragma once

#include "log_query/filter.h"

#include <string>

namespace log_query {

class CompareFilter : public Filter {
public:
    enum class Op {
        Greater,
        GreaterEqual,
        Less,
        LessEqual,
        NotEqual
    };

    CompareFilter(std::string field, Op op, double threshold);

    bool match(const LogLine& line) const override;
    std::string describe() const override;

private:
    static double to_double(const FieldValue& value);

    std::string field_;
    Op op_;
    double threshold_;
};

} // namespace log_query
