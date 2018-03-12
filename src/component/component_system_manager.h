#pragma once

#include "../common.h"
#include "../chunk.h"
#include "../mesh.h"
#include "../render_data.h"
#include "../ship_space.h"
#include "../player.h"

#include "component_managers.h"

struct component_system_manager {
    component_system_manager() = default;

    virtual ~component_system_manager() = default;

    component_managers managers{};
};

extern const char *comms_msg_type_switch_state;
extern const char *comms_msg_type_pressure_sensor_1_state;
extern const char *comms_msg_type_pressure_sensor_2_state;
extern const char *comms_msg_type_sensor_comparison_state;
extern const char *comms_msg_type_proximity_sensor_state;

void
tick_gas_producers(ship_space *ship);

void
tick_doors(ship_space *ship);

void
tick_power_consumers(ship_space *ship);

void
tick_light_components(ship_space *ship);

void
tick_pressure_sensors(ship_space *ship);

void
tick_sensor_comparators(ship_space *ship);

void
tick_readers(ship_space *ship);

void
tick_proximity_sensors(ship_space *ship, player *pl);

void
draw_renderables(frame_data *frame);

void
draw_doors(frame_data *frame);

void
build_absolute_transforms();