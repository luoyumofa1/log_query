#pragma once

#include <string>

namespace log_query {
namespace ansi {

constexpr auto RESET   = "\033[0m";
constexpr auto RED     = "\033[31m";
constexpr auto GREEN   = "\033[32m";
constexpr auto YELLOW  = "\033[33m";
constexpr auto BLUE    = "\033[34m";
constexpr auto MAGENTA = "\033[35m";
constexpr auto CYAN    = "\033[36m";
constexpr auto WHITE   = "\033[37m";
constexpr auto BOLD    = "\033[1m";
constexpr auto DIM     = "\033[2m";
constexpr auto GRAY    = "\033[90m";
constexpr auto BG_RED  = "\033[41m";

inline std::string colorize(const std::string& text, const char* color) {
    return std::string(color) + text + RESET;
}

} // namespace ansi
} // namespace log_query
