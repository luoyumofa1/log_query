#include "color_renderer.h"
#include "util/ansi.h"

#include <algorithm>
#include <cctype>
#include <iostream>

namespace log_query {

ColorRenderer::ColorRenderer(std::string level_field)
    : level_field_(std::move(level_field))
{
}

void ColorRenderer::render_line(const LogLine& line) {
    std::string level = "INFO";
    if (!level_field_.empty()) {
        auto it = line.fields.find(level_field_);
        if (it != line.fields.end() && std::holds_alternative<std::string>(it->second)) {
            level = std::get<std::string>(it->second);
        }
    }

    const char* color = level_color(level);

    std::string formatted = format_line(line);
    std::cout << ansi::colorize(formatted, color) << "\n";
}

const char* ColorRenderer::level_color(const std::string& level) {
    std::string upper(level);
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    if (upper == "FATAL") return ansi::BG_RED;
    if (upper == "ERROR") return ansi::RED;
    if (upper == "WARN")  return ansi::YELLOW;
    if (upper == "INFO")  return ansi::WHITE;
    if (upper == "DEBUG") return ansi::GRAY;
    if (upper == "TRACE") return ansi::GRAY;
    return ansi::WHITE;
}

std::string ColorRenderer::format_line(const LogLine& line) {
    return line.raw_text;
}

} // namespace log_query
