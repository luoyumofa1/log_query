#pragma once

#include "config/log_format.h"
#include "log_query/log_line.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace log_query {

struct Token {
    enum Type { FIXED, FIELD };
    Type type;
    std::string value;
};

class LineParser {
public:
    explicit LineParser(const LogFormatConfig& config);

    std::optional<LogLine> parse(std::string_view raw, int64_t line_number) const;

private:
    size_t find_next_fixed(std::string_view raw, size_t start, size_t token_idx) const;

    std::vector<Token> tokens_;
    LogFormatConfig config_;
};

} // namespace log_query
