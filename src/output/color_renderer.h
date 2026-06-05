#pragma once

#include "log_query/renderer.h"

#include <string>

namespace log_query {

class ColorRenderer : public Renderer {
public:
    explicit ColorRenderer(std::string level_field);

    void render_line(const LogLine& line) override;

private:
    static const char* level_color(const std::string& level);
    static std::string format_line(const LogLine& line);

    std::string level_field_;
};

} // namespace log_query
