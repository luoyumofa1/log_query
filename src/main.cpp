#include "config/log_format.h"
#include "filter/field_filter.h"
#include "filter/filter_chain.h"
#include "output/color_renderer.h"
#include "parser/line_parser.h"
#include "util/file_reader.h"

#include <CLI11.hpp>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char** argv) {
    CLI::App app{"log-query — structured log query tool"};

    std::string log_file = "-";
    std::string format_config;
    std::string module_filter;
    std::string level_filter;
    std::string output_mode = "color";

    app.add_option("file", log_file, "Log file path (use - for stdin)");
    app.add_option("--format-config", format_config,
                   "Path to log format JSON config file");
    app.add_option("--module", module_filter, "Filter by module name");
    app.add_option("--level", level_filter, "Filter by log level");
    app.add_option("--output", output_mode, "Output mode: color, json, csv");

    CLI11_PARSE(app, argc, argv);

    try {
        std::string config_path = format_config;
        if (config_path.empty()) {
            config_path = "config/adas_default.json";
        }

        auto format = log_query::load_format_config(config_path);

        log_query::LineParser parser(format);

        log_query::FilterChain chain;
        if (!module_filter.empty()) {
            chain.add(std::make_unique<log_query::FieldEqualFilter>("module", module_filter));
        }
        if (!level_filter.empty()) {
            chain.add(std::make_unique<log_query::FieldEqualFilter>("level", level_filter));
        }

        std::unique_ptr<log_query::Renderer> renderer;
        if (output_mode == "color") {
            renderer = std::make_unique<log_query::ColorRenderer>();
        } else {
            std::cerr << "Warning: output mode '" << output_mode
                      << "' not yet implemented, falling back to color\n";
            renderer = std::make_unique<log_query::ColorRenderer>();
        }

        log_query::FileReader reader(log_file);
        int64_t line_number = 0;

        renderer->render_header();

        for (auto& raw_line : reader) {
            ++line_number;
            auto parsed = parser.parse(raw_line, line_number);
            if (parsed && chain.match(*parsed)) {
                renderer->render_line(*parsed);
            }
        }

        renderer->render_footer();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
