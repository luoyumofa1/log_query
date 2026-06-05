#include "line_parser.h"
#include "field.h"

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
            if (pos + tok.value.size() > raw.size()) return std::nullopt;
            if (raw.substr(pos, tok.value.size()) != tok.value) return std::nullopt;
            pos += tok.value.size();
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
            auto found = raw.find(tokens_[i].value, start);
            if (found != std::string_view::npos) {
                return found;
            }
        }
    }
    return raw.size();
}

} // namespace log_query
