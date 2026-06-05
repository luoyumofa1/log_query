#pragma once

#include "log_query/log_line.h"

#include <string>

namespace log_query {

class Filter {
public:
    virtual ~Filter() = default;
    virtual bool match(const LogLine& line) const = 0;
    virtual std::string describe() const = 0;
};

} // namespace log_query
