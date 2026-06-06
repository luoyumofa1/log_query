#pragma once

#include "log_query/filter.h"

#include <regex>
#include <string>

namespace log_query {

class RegexFilter : public Filter {
public:
    RegexFilter(std::string field, std::string pattern);

    bool match(const LogLine& line) const override;
    std::string describe() const override;

private:
    std::string field_;
    std::regex regex_;
    std::string pattern_;
};

} // namespace log_query
