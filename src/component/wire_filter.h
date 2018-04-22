#pragma once

#include <string>
#include <array>

#include "../enums/enums.h"

struct filter_ui_state {
    std::string component_name;
    int field_id;
    std::array<char, 256> filter;
    msg_type msg_type;
};

struct wire_filter_ptr
{
    std::string *wrapped = nullptr;
    msg_type msg_type{};
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

    void set(filter_ui_state const &s) {
        msg_type = s.msg_type;
        if (!wrapped) {
            wrapped = new std::string(s.filter.data());
        }
        else {
            *wrapped = s.filter.data();
        }
    }
};
