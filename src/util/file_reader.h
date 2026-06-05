#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace log_query {

class FileReader {
public:
    explicit FileReader(const std::string& path) {
        if (path == "-" || path.empty()) {
            use_stdin_ = true;
        } else {
            file_.open(path);
            if (!file_.is_open()) {
                throw std::runtime_error("Failed to open file: " + path);
            }
        }
    }

    class Iterator {
    public:
        using value_type = std::string;
        using difference_type = std::ptrdiff_t;
        using pointer = const std::string*;
        using reference = const std::string&;
        using iterator_category = std::input_iterator_tag;

        Iterator() : reader_(nullptr), done_(true) {}

        explicit Iterator(FileReader* reader) : reader_(reader), done_(false) {
            advance();
        }

        const std::string& operator*() const { return current_; }
        const std::string* operator->() const { return &current_; }

        Iterator& operator++() {
            advance();
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            advance();
            return tmp;
        }

        bool operator==(const Iterator& other) const {
            if (done_ && other.done_) return true;
            return reader_ == other.reader_ && done_ == other.done_;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        void advance() {
            if (!reader_) {
                done_ = true;
                return;
            }
            if (reader_->use_stdin_) {
                if (!std::getline(std::cin, current_)) {
                    done_ = true;
                }
            } else {
                if (!std::getline(reader_->file_, current_)) {
                    done_ = true;
                }
            }
        }

        FileReader* reader_;
        std::string current_;
        bool done_;
    };

    Iterator begin() { return Iterator(this); }
    Iterator end() { return Iterator(); }

private:
    std::ifstream file_;
    bool use_stdin_ = false;
};

} // namespace log_query
