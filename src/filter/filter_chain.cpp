#include "filter_chain.h"

namespace log_query {

void FilterChain::add(std::unique_ptr<Filter> filter) {
    filters_.push_back(std::move(filter));
}

bool FilterChain::match(const LogLine& line) const {
    for (auto& f : filters_) {
        if (!f->match(line)) return false;
    }
    return true;
}

} // namespace log_query
