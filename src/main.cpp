#include "config/log_format.h"
#include "filter/field_filter.h"
#include "filter/filter_chain.h"
#include "filter/regex_filter.h"
#include "filter/time_range_filter.h"
#include "output/color_renderer.h"
#include "output/csv_renderer.h"
#include "output/json_renderer.h"
#include "output/plain_renderer.h"
#include "output/summary_renderer.h"
#include "parser/line_parser.h"
#include "util/file_reader.h"
#include "util/time_parse.h"

#include <CLI11.hpp>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

static std::pair<std::string, std::string> parse_filter(const std::string& raw) {
    auto eq = raw.find('=');
    if (eq == std::string::npos) {
        throw std::runtime_error("Invalid filter format: '" + raw +
                                 "'. Expected field=value");
    }
    return {raw.substr(0, eq), raw.substr(eq + 1)};
}

static std::string make_timestamped_filename(const std::string& ext) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif
    std::ostringstream oss;
    oss << "log-query-result-"
        << std::setfill('0')
        << std::setw(4) << (tm_now.tm_year + 1900)
        << std::setw(2) << (tm_now.tm_mon + 1)
        << std::setw(2) << tm_now.tm_mday
        << "-"
        << std::setw(2) << tm_now.tm_hour
        << std::setw(2) << tm_now.tm_min
        << std::setw(2) << tm_now.tm_sec
        << "." << ext;
    return oss.str();
}

int main(int argc, char** argv) {
    CLI::App app{"log-query — structured log query tool"};

    std::string log_file = "-";
    std::string format_config;
    std::vector<std::string> filters;
    std::string from_time;
    std::string to_time;
    std::vector<std::string> match_filters;
    std::string output_mode = "color";
    std::string output_file;

    app.add_option("file", log_file, "Log file path (use - for stdin)");
    app.add_option("--format-config", format_config,
                   "Path to log format JSON config file");
    app.add_option("-f,--filter", filters,
                   "Field filter (field=value, repeatable)");
    app.add_option("--from", from_time,
                   "Start time (YYYY-MM-DD or YYYY-MM-DD HH:MM:SS)");
    app.add_option("--to", to_time,
                   "End time (YYYY-MM-DD or YYYY-MM-DD HH:MM:SS)");
    app.add_option("-m,--match", match_filters,
                   "Regex filter (field=pattern, repeatable)");
    app.add_option("--output", output_mode, "Output mode: color, plain, summary, json, csv");
    app.add_option("--output-file", output_file,
                   "Output file path (for json/csv mode, auto-generated if omitted)");

    CLI11_PARSE(app, argc, argv);

    try {
        std::string config_path = format_config;
        if (config_path.empty()) {
            config_path = "config/adas_default.json";
        }

        auto format = log_query::load_format_config(config_path);

        log_query::LineParser parser(format);

        log_query::FilterChain chain;
        for (auto& f : filters) {
            auto [field, value] = parse_filter(f);
            chain.add(std::make_unique<log_query::FieldEqualFilter>(field, value));
        }

        if (!from_time.empty() || !to_time.empty()) {
            auto from = from_time.empty()
                ? std::chrono::system_clock::time_point{}
                : log_query::parse_user_time(from_time);
            auto to = to_time.empty()
                ? std::chrono::system_clock::time_point::max()
                : log_query::parse_user_time(to_time);
            chain.add(std::make_unique<log_query::TimeRangeFilter>(from, to));
        }

        for (auto& m : match_filters) {
            auto [field, pattern] = parse_filter(m);
            chain.add(std::make_unique<log_query::RegexFilter>(field, pattern));
        }

        std::unique_ptr<std::ofstream> file_stream;
        std::unique_ptr<log_query::Renderer> renderer;

        if (output_mode == "json") {
            std::string path = output_file;
            if (path.empty()) path = make_timestamped_filename("json");
            file_stream = std::make_unique<std::ofstream>(path);
            if (!file_stream->is_open()) {
                throw std::runtime_error("Failed to create output file: " + path);
            }
            renderer = std::make_unique<log_query::JsonRenderer>(*file_stream);
            std::cerr << "Writing JSON to: " << path << "\n";
        } else if (output_mode == "csv") {
            std::string path = output_file;
            if (path.empty()) path = make_timestamped_filename("csv");
            file_stream = std::make_unique<std::ofstream>(path);
            if (!file_stream->is_open()) {
                throw std::runtime_error("Failed to create output file: " + path);
            }
            renderer = std::make_unique<log_query::CsvRenderer>(*file_stream);
            std::cerr << "Writing CSV to: " << path << "\n";
        } else if (output_mode == "plain") {
            renderer = std::make_unique<log_query::PlainRenderer>();
        } else if (output_mode == "summary") {
            std::vector<std::string> level_order;
            auto it = format.fields.find(format.level_field);
            if (it != format.fields.end()) {
                level_order = it->second.enum_values;
            }
            renderer = std::make_unique<log_query::SummaryRenderer>(
                format.level_field, format.module_field, std::move(level_order));
        } else {
            renderer = std::make_unique<log_query::ColorRenderer>(format.level_field);
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
