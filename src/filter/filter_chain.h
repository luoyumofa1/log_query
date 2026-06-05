#pragma once

#include "log_query/filter.h"

#include <memory>
#include <vector>

namespace log_query {

class FilterChain {
public:
    void add(std::unique_ptr<Filter> filter);

    bool match(const LogLine& line) const;

    size_t size() const { return filters_.size(); }

private:
    std::vector<std::unique_ptr<Filter>> filters_;
};

} // namespace log_query
