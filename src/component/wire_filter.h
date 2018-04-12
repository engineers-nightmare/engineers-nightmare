#pragma once

#include <string>

struct wire_filter_ptr
{
    std::string *wrapped = nullptr;
    wire_filter_ptr(const wire_filter_ptr&) = delete;
    wire_filter_ptr& operator=(const wire_filter_ptr&) = default;

    ~wire_filter_ptr() {
        delete wrapped;
    }

    explicit wire_filter_ptr(std::string& s) {
        if (wrapped == nullptr) {
            wrapped = new std::string;
        }
        *wrapped = s;
    }

    wire_filter_ptr(std::initializer_list<std::string>): wrapped(nullptr) {
        // bogus
    }

    const char * c_str() const {
        if (wrapped) {
            return wrapped->c_str();
        }
        else {
            return nullptr;
        }
    }

    void set(const char * string) {
        if (!wrapped) {
            wrapped = new std::string(string);
        }
        else {
            *wrapped = string;
        }
    }
};
