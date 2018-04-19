#pragma once

#include <vector>

#include "../component/c_entity.h"
#include "../enums/enums.h"

enum wire_type {
    wire_type_power = 0,
    wire_type_comms = 1,

    num_wire_types
};

extern char const *wire_type_names[num_wire_types];

struct power_wiring_data {
    float total_power = 0;
    float total_draw = 0;
    float peak_draw = 0;
    unsigned num_consumers = 0;
    unsigned num_providers = 0;
};

struct comms_msg {
    c_entity originator{0};
    msg_type type;

    /* todo: other data types */
    float data = 0.0f;
};


struct comms_wiring_data {
    std::vector<comms_msg> read_buffer{}, write_buffer{};
};


/* todo: the rest of this */
//struct fluid_wiring_data {
//    unsigned total_power;
//    unsigned total_draw;
//    unsigned num_consumers;
//    unsigned fluid_type;
//};
