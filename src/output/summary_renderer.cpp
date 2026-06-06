#include "summary_renderer.h"
#include "util/ansi.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace log_query {

SummaryRenderer::SummaryRenderer(std::string level_field,
                                 std::string module_field,
                                 std::vector<std::string> level_order)
    : Renderer(std::cout)
    , level_field_(std::move(level_field))
    , module_field_(std::move(module_field))
    , level_order_(std::move(level_order))
{
}

std::string SummaryRenderer::get_field_string(const LogLine& line,
                                               const std::string& field) const {
    auto it = line.fields.find(field);
    if (it != line.fields.end() && std::holds_alternative<std::string>(it->second)) {
        return std::get<std::string>(it->second);
    }
    return "";
}

void SummaryRenderer::render_line(const LogLine& line) {
    std::string module = get_field_string(line, module_field_);
    std::string level = get_field_string(line, level_field_);

    if (module.empty()) module = "(unknown)";
    if (level.empty()) level = "(unknown)";

    stats_[module][level]++;
    total_lines_++;
}

void SummaryRenderer::print_separator() {
    os_ << std::string(70, '-') << "\n";
}

void SummaryRenderer::print_row(const std::string& module,
                                 const std::map<std::string, int>& level_counts,
                                 const std::vector<std::string>& all_levels) {
    os_ << std::left << std::setw(18) << module;
    int row_total = 0;
    for (auto& lvl : all_levels) {
        auto it = level_counts.find(lvl);
        int count = (it != level_counts.end()) ? it->second : 0;
        row_total += count;
        os_ << std::right << std::setw(7) << count;
    }
    os_ << std::right << std::setw(7) << row_total << "\n";
}

void SummaryRenderer::render_footer() {
    if (stats_.empty()) {
        os_ << "No matching log lines found.\n";
        return;
    }

    std::vector<std::string> all_levels = level_order_;
    for (auto& [module, level_counts] : stats_) {
        for (auto& [lvl, count] : level_counts) {
            if (std::find(all_levels.begin(), all_levels.end(), lvl) == all_levels.end()) {
                all_levels.push_back(lvl);
            }
        }
    }

    os_ << std::left << std::setw(18) << "Module";
    for (auto& lvl : all_levels) {
        os_ << std::right << std::setw(7) << lvl;
    }
    os_ << std::right << std::setw(7) << "Total" << "\n";

    print_separator();

    std::map<std::string, int> grand_totals;
    for (auto& lvl : all_levels) {
        grand_totals[lvl] = 0;
    }

    for (auto& [module, level_counts] : stats_) {
        print_row(module, level_counts, all_levels);
        for (auto& lvl : all_levels) {
            auto it = level_counts.find(lvl);
            if (it != level_counts.end()) {
                grand_totals[lvl] += it->second;
            }
        }
    }

    print_separator();

    os_ << std::left << std::setw(18) << "Total";
    int grand_total = 0;
    for (auto& lvl : all_levels) {
        grand_total += grand_totals[lvl];
        os_ << std::right << std::setw(7) << grand_totals[lvl];
    }
    os_ << std::right << std::setw(7) << grand_total << "\n";
}

} // namespace log_query
