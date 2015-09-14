#pragma once
#include <vector>

enum wire_type {
    wire_type_power = 0,

    num_wire_types
};

extern std::vector<wire_type> wire_types;


struct power_wiring_data {
    unsigned total_power = 0;
    unsigned total_draw = 0;
    unsigned peak_draw = 0;
    unsigned num_consumers = 0;
    unsigned num_providers = 0;
};


/* todo: the rest of this */
//struct comms_wiring_data {
//    std::vector<comms_data> msg_buffer;
//    unsigned num_connected;
//};


/* todo: the rest of this */
//struct fluid_wiring_data {
//    unsigned total_power;
//    unsigned total_draw;
//    unsigned num_consumers;
//    unsigned fluid_type;
//};
