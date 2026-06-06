#pragma once

#include "log_query/renderer.h"

#include <map>
#include <string>
#include <vector>

namespace log_query {

class SummaryRenderer : public Renderer {
public:
    SummaryRenderer(std::string level_field,
                    std::string module_field,
                    std::vector<std::string> level_order);

    void render_line(const LogLine& line) override;
    void render_footer() override;

private:
    std::string get_field_string(const LogLine& line, const std::string& field) const;
    void print_separator();
    void print_row(const std::string& module,
                   const std::map<std::string, int>& level_counts,
                   const std::vector<std::string>& all_levels);

    std::string level_field_;
    std::string module_field_;
    std::vector<std::string> level_order_;
    std::map<std::string, std::map<std::string, int>> stats_;
    int64_t total_lines_ = 0;
};

} // namespace log_query
