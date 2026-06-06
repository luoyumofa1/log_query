#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace log_query {

inline std::chrono::system_clock::time_point parse_user_time(const std::string& raw) {
    std::tm tm = {};
    std::istringstream ss(raw);

    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        ss.clear();
        ss.str(raw);
        ss >> std::get_time(&tm, "%Y-%m-%d");
        if (ss.fail()) {
            throw std::runtime_error("Invalid time format: '" + raw +
                                     "'. Expected 'YYYY-MM-DD HH:MM:SS' or 'YYYY-MM-DD'");
        }
    }

#ifdef _WIN32
    auto t = _mkgmtime(&tm);
#else
    auto t = timegm(&tm);
#endif
    return std::chrono::system_clock::from_time_t(t);
}

} // namespace log_query
