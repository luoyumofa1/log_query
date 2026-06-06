#pragma once

#include "log_query/renderer.h"

#include <string>
#include <vector>

namespace log_query {

class JsonRenderer : public Renderer {
public:
    explicit JsonRenderer(std::ostream& os);

    void render_header() override;
    void render_line(const LogLine& line) override;
    void render_footer() override;

private:
    bool first_ = true;
    std::vector<std::string> field_order_;
};

} // namespace log_query
