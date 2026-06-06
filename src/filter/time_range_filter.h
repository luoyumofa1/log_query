#pragma once

#include "log_query/filter.h"

#include <chrono>
#include <string>

namespace log_query {

class TimeRangeFilter : public Filter {
public:
    TimeRangeFilter(std::chrono::system_clock::time_point from,
                    std::chrono::system_clock::time_point to);

    bool match(const LogLine& line) const override;
    std::string describe() const override;

private:
    std::chrono::system_clock::time_point from_;
    std::chrono::system_clock::time_point to_;
};

} // namespace log_query
