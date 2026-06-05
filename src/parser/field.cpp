#include "field.h"
#include "config/log_format.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace log_query {

std::optional<FieldValue> convert_field(
    std::string_view raw,
    FieldType type,
    const std::string& datetime_format,
    const std::vector<std::string>& enum_values)
{
    switch (type) {
    case FieldType::String:
        return std::string(raw);

    case FieldType::Int: {
        std::string s(raw);
        try {
            size_t pos = 0;
            int64_t val = std::stoll(s, &pos);
            if (pos != s.size()) return std::nullopt;
            return val;
        } catch (...) {
            return std::nullopt;
        }
    }

    case FieldType::Float: {
        std::string s(raw);
        try {
            size_t pos = 0;
            double val = std::stod(s, &pos);
            if (pos != s.size()) return std::nullopt;
            return val;
        } catch (...) {
            return std::nullopt;
        }
    }

    case FieldType::DateTime: {
        auto ts = parse_timestamp(raw, datetime_format);
        if (ts == 0) return std::nullopt;
        return std::chrono::system_clock::from_time_t(ts);
    }

    case FieldType::Enum: {
        std::string upper(raw);
        std::transform(upper.begin(), upper.end(), upper.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        for (auto& v : enum_values) {
            std::string v_upper = v;
            std::transform(v_upper.begin(), v_upper.end(), v_upper.begin(),
                           [](unsigned char c) { return std::toupper(c); });
            if (upper == v_upper) return v;
        }
        return std::nullopt;
    }

    case FieldType::Regex:
        return std::string(raw);
    }

    return std::nullopt;
}

int64_t parse_timestamp(std::string_view raw, const std::string& format) {
    std::string s(raw);
    std::tm tm = {};
    std::istringstream ss(s);

    if (format.find("%f") != std::string::npos) {
        auto dot_pos = s.find('.');
        if (dot_pos != std::string::npos) {
            std::string before_dot = s.substr(0, dot_pos);
            std::istringstream bss(before_dot);
            bss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            if (bss.fail()) return 0;
        } else {
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            if (ss.fail()) return 0;
        }
    } else {
        ss >> std::get_time(&tm, format.c_str());
        if (ss.fail()) return 0;
    }

#ifdef _WIN32
    return _mkgmtime(&tm);
#else
    return timegm(&tm);
#endif
}

int compare_enum(const std::string& value, const std::vector<std::string>& values) {
    std::string upper(value);
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    for (size_t i = 0; i < values.size(); ++i) {
        std::string v_upper = values[i];
        std::transform(v_upper.begin(), v_upper.end(), v_upper.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        if (upper == v_upper) return static_cast<int>(i);
    }
    return -1;
}

} // namespace log_query
