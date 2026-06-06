#include "regex_filter.h"

namespace log_query {

RegexFilter::RegexFilter(std::string field, std::string pattern)
    : field_(std::move(field))
    , regex_(pattern, std::regex::ECMAScript | std::regex::icase)
    , pattern_(std::move(pattern))
{
}

bool RegexFilter::match(const LogLine& line) const {
    auto it = line.fields.find(field_);
    if (it == line.fields.end()) return false;

    if (!std::holds_alternative<std::string>(it->second)) return false;

    const auto& val = std::get<std::string>(it->second);
    return std::regex_search(val, regex_);
}

std::string RegexFilter::describe() const {
    return field_ + " =~ /" + pattern_ + "/";
}

} // namespace log_query
