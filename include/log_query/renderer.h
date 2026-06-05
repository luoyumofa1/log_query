#pragma once

#include "log_query/log_line.h"

namespace log_query {

class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void render_header() {}
    virtual void render_line(const LogLine& line) = 0;
    virtual void render_footer() {}
};

} // namespace log_query
