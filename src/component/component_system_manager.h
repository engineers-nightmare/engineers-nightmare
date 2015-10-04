#pragma once

#include "../common.h"
#include "../chunk.h"
#include "../mesh.h"
#include "../render_data.h"
#include "../ship_space.h"
#include "sensor_comparator_component.h"
#include "gas_production_component.h"
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

extern sensor_comparator_component_manager comparator_man;
extern gas_production_component_manager gas_man;
extern light_component_manager light_man;
extern physics_component_manager physics_man;
extern relative_position_component_manager pos_man;
extern power_component_manager power_man;
extern power_provider_component_manager power_provider_man;
extern pressure_sensor_component_manager pressure_man;
extern renderable_component_manager render_man;
extern surface_attachment_component_manager surface_man;
extern switch_component_manager switch_man;
extern type_component_manager type_man;
extern door_component_manager door_man;
extern reader_component_manager reader_man;

extern const char * comms_msg_type_switch_state;
extern const char * comms_msg_type_pressure_sensor_1_state;
extern const char * comms_msg_type_pressure_sensor_2_state;
extern const char * comms_msg_type_sensor_comparison_state;


void
tick_gas_producers(ship_space * ship);

void
tick_doors(ship_space * ship);

void
tick_power_consumers(ship_space * ship);

void
tick_light_components(ship_space * ship);

void
tick_pressure_sensors(ship_space * ship);

void
tick_sensor_comparators(ship_space* ship);

void
draw_renderables(frame_data *frame);

void
draw_doors(frame_data *frame);
