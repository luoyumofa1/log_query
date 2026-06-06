#include "json_renderer.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace log_query {

static std::string escape_json(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:   out += c;      break;
        }
    }
    return out;
}

static std::string field_to_json_string(const FieldValue& val) {
    if (std::holds_alternative<std::string>(val)) {
        return "\"" + escape_json(std::get<std::string>(val)) + "\"";
    }
    if (std::holds_alternative<int64_t>(val)) {
        return std::to_string(std::get<int64_t>(val));
    }
    if (std::holds_alternative<double>(val)) {
        return std::to_string(std::get<double>(val));
    }
    if (std::holds_alternative<std::chrono::system_clock::time_point>(val)) {
        auto t = std::chrono::system_clock::to_time_t(
            std::get<std::chrono::system_clock::time_point>(val));
        std::ostringstream oss;
        oss << "\"" << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ") << "\"";
        return oss.str();
    }
    return "null";
}

void JsonRenderer::render_header() {
    std::cout << "[\n";
    first_ = true;
}

void JsonRenderer::render_line(const LogLine& line) {
    if (field_order_.empty()) {
        for (auto& [k, v] : line.fields) {
            field_order_.push_back(k);
        }
    }

    if (!first_) {
        std::cout << ",\n";
    }
    first_ = false;

    std::cout << "  {\"line\":" << line.line_number;
    for (auto& key : field_order_) {
        auto it = line.fields.find(key);
        if (it != line.fields.end()) {
            std::cout << ",\"" << escape_json(key) << "\":" << field_to_json_string(it->second);
        }
    }
    std::cout << "}";
}

void JsonRenderer::render_footer() {
    if (!first_) {
        std::cout << "\n";
    }
    std::cout << "]\n";
}

} // namespace log_query
