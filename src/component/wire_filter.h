#pragma once

#include <string>
#include <array>

#include "../enums/enums.h"

struct filter_ui_state {
    c_entity entity;
    std::string component_name;
    int field_id;
    std::array<char, 256> filter;
    msg_type type;
};

struct output_ui_state {
    c_entity entity;
    std::string component_name;
    std::array<char, 256> output;
};

struct wire_filter_ptr
{
    std::string *wrapped = nullptr;
    msg_type type{};
    wire_filter_ptr(const wire_filter_ptr&) = delete;

    wire_filter_ptr& operator=(const wire_filter_ptr &w) {
        type = w.type;
        if (!wrapped && w.wrapped) {
            wrapped = new std::string(*w.wrapped);
        }
        else if (wrapped && w.wrapped) {
            *wrapped = *w.wrapped;
        }
        else if (wrapped && !w.wrapped) {
            delete wrapped;
            wrapped = nullptr;
        }
        return *this;
    }

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
        type = s.type;
        if (!wrapped) {
            wrapped = new std::string(s.filter.data());
        }
        else {
            *wrapped = s.filter.data();
        }
    }
};
