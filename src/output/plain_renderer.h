#pragma once

#include "log_query/renderer.h"

#include <string>

namespace log_query {

class PlainRenderer : public Renderer {
public:
    explicit PlainRenderer();

    void render_line(const LogLine& line) override;
};

} // namespace log_query
