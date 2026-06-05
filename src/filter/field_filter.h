#pragma once

#include "log_query/filter.h"

#include <string>

namespace log_query {

class FieldEqualFilter : public Filter {
public:
    FieldEqualFilter(std::string field, std::string value);

    bool match(const LogLine& line) const override;
    std::string describe() const override;

private:
    std::string field_;
    std::string value_;
};

} // namespace log_query
