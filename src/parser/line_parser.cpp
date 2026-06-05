#include "line_parser.h"
#include "field.h"

#include <cctype>
#include <stdexcept>

namespace log_query {

static std::vector<Token> tokenize(const std::string& pattern) {
    std::vector<Token> tokens;
    size_t pos = 0;

    while (pos < pattern.size()) {
        if (pattern[pos] == '{') {
            size_t end = pattern.find('}', pos);
            if (end == std::string::npos) {
                throw std::runtime_error("Unmatched '{' in pattern: " + pattern);
            }
            std::string field_name = pattern.substr(pos + 1, end - pos - 1);
            tokens.push_back({Token::FIELD, field_name});
            pos = end + 1;
        } else {
            size_t next_brace = pattern.find('{', pos);
            if (next_brace == std::string::npos) {
                tokens.push_back({Token::FIXED, pattern.substr(pos)});
                break;
            } else {
                tokens.push_back({Token::FIXED, pattern.substr(pos, next_brace - pos)});
                pos = next_brace;
            }
        }
    }

    return tokens;
}

static bool match_fixed_flexible(std::string_view raw, size_t& pos, std::string_view fixed) {
    size_t fi = 0;
    while (fi < fixed.size()) {
        if (pos >= raw.size()) return false;

        if (std::isspace(static_cast<unsigned char>(fixed[fi]))) {
            while (pos < raw.size() && std::isspace(static_cast<unsigned char>(raw[pos]))) {
                ++pos;
            }
            while (fi < fixed.size() && std::isspace(static_cast<unsigned char>(fixed[fi]))) {
                ++fi;
            }
        } else {
            if (raw[pos] != fixed[fi]) return false;
            ++pos;
            ++fi;
        }
    }
    return true;
}

LineParser::LineParser(const LogFormatConfig& config)
    : tokens_(tokenize(config.pattern))
    , config_(config)
{
}

std::optional<LogLine> LineParser::parse(std::string_view raw, int64_t line_number) const {
    LogLine line;
    line.line_number = line_number;
    line.raw_text = std::string(raw);
    size_t pos = 0;

    for (size_t i = 0; i < tokens_.size(); ++i) {
        auto& tok = tokens_[i];

        if (tok.type == Token::FIXED) {
            if (!match_fixed_flexible(raw, pos, tok.value)) return std::nullopt;
        } else {
            size_t end;
            if (i + 1 < tokens_.size()) {
                end = find_next_fixed(raw, pos, i);
            } else {
                end = raw.size();
            }

            std::string_view field_value = raw.substr(pos, end - pos);

            auto it = config_.fields.find(tok.value);
            if (it == config_.fields.end()) {
                pos = end;
                continue;
            }

            auto& fc = it->second;
            auto converted = convert_field(field_value, fc.type,
                                           fc.datetime_format, fc.enum_values);
            if (converted) {
                line.fields[tok.value] = *converted;
                if (fc.type == FieldType::DateTime) {
                    line.timestamp = std::get<std::chrono::system_clock::time_point>(*converted);
                }
            }

            pos = end;
        }
    }

    return line;
}

size_t LineParser::find_next_fixed(std::string_view raw, size_t start, size_t token_idx) const {
    for (size_t i = token_idx + 1; i < tokens_.size(); ++i) {
        if (tokens_[i].type == Token::FIXED) {
            auto& fixed = tokens_[i].value;
            size_t fi = 0;
            while (fi < fixed.size() && std::isspace(static_cast<unsigned char>(fixed[fi]))) {
                ++fi;
            }
            if (fi < fixed.size()) {
                auto found = raw.find(fixed[fi], start);
                if (found != std::string_view::npos) {
                    return found;
                }
            }
        }
    }
    return raw.size();
}

} // namespace log_query
