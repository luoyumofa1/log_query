#pragma once

#include "log_query/renderer.h"

#include <string>
#include <vector>

namespace log_query {

class CsvRenderer : public Renderer {
public:
    explicit CsvRenderer(std::ostream& os);

    void render_header() override;
    void render_line(const LogLine& line) override;
    void render_footer() override;

private:
    bool header_written_ = false;
    std::vector<std::string> field_order_;

    static std::string escape_csv(const std::string& s);
};

} // namespace log_query
