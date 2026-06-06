#include "csv_renderer.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace log_query {

std::string CsvRenderer::escape_csv(const std::string& s) {
    bool needs_quote = false;
    for (char c : s) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') {
            needs_quote = true;
            break;
        }
    }
    if (!needs_quote) return s;

    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += '"';
    return out;
}

static std::string field_to_csv_string(const FieldValue& val) {
    if (std::holds_alternative<std::string>(val)) {
        return std::get<std::string>(val);
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
        oss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
        return oss.str();
    }
    return "";
}

void CsvRenderer::render_header() {
    header_written_ = false;
}

void CsvRenderer::render_line(const LogLine& line) {
    if (!header_written_) {
        for (auto& [k, v] : line.fields) {
            field_order_.push_back(k);
        }
        std::cout << "line";
        for (auto& key : field_order_) {
            std::cout << "," << escape_csv(key);
        }
        std::cout << "\n";
        header_written_ = true;
    }

    std::cout << line.line_number;
    for (auto& key : field_order_) {
        auto it = line.fields.find(key);
        if (it != line.fields.end()) {
            std::cout << "," << escape_csv(field_to_csv_string(it->second));
        } else {
            std::cout << ",";
        }
    }
    std::cout << "\n";
}

void CsvRenderer::render_footer() {
}

} // namespace log_query
