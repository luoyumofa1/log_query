#include "plain_renderer.h"

#include <iostream>

namespace log_query {

PlainRenderer::PlainRenderer()
    : Renderer(std::cout)
{
}

void PlainRenderer::render_line(const LogLine& line) {
    os_ << line.raw_text << "\n";
}

} // namespace log_query
