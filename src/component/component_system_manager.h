#pragma once

#include "../common.h"
#include "../chunk.h"
#include "../mesh.h"
#include "../render_data.h"
#include "../ship_space.h"
#include "../player.h"
#include "sensor_comparator_component.h"
#include "gas_producer_component.h"
#include "light_component.h"
#include "physics_component.h"
#include "power_component.h"
#include "power_provider_component.h"
#include "pressure_sensor_component.h"
#include "relative_position_component.h"
#include "renderable_component.h"
#include "surface_attachment_component.h"
#include "switch_component.h"
#include "type_component.h"
#include "door_component.h"
#include "reader_component.h"
#include "proximity_sensor_component.h"

void initialize_component_managers();

extern std::unordered_map<std::string, std::unique_ptr<component_manager>> component_managers;

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
set_door_state(ship_space *ship, c_entity ce, surface_type s);
