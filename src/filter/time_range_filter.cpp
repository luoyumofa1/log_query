#include "time_range_filter.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace log_query {

TimeRangeFilter::TimeRangeFilter(
    std::chrono::system_clock::time_point from,
    std::chrono::system_clock::time_point to)
    : from_(from)
    , to_(to)
{
}

bool TimeRangeFilter::match(const LogLine& line) const {
    auto ts = line.timestamp;
    if (ts == std::chrono::system_clock::time_point{}) return true;
    return ts >= from_ && ts <= to_;
}

std::string TimeRangeFilter::describe() const {
    auto from_t = std::chrono::system_clock::to_time_t(from_);
    auto to_t = std::chrono::system_clock::to_time_t(to_);
    std::ostringstream oss;
    oss << "time in ["
        << std::put_time(std::gmtime(&from_t), "%Y-%m-%d %H:%M:%S")
        << ", "
        << std::put_time(std::gmtime(&to_t), "%Y-%m-%d %H:%M:%S")
        << "]";
    return oss.str();
}

} // namespace log_query
