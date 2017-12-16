#pragma once

#include "../block.h"
#include "../common.h"

struct player;
struct raycast_info_block;
struct frame_data;


struct tool
{
    virtual ~tool() = default;

    virtual void use(raycast_info *rc) = 0;
    virtual void alt_use(raycast_info *rc) = 0;
    virtual void long_use(raycast_info *rc) = 0;
    virtual void long_alt_use(raycast_info *rc) {}
    virtual void select() {}
    virtual void unselect() {}

    virtual void cycle_mode() = 0;

    virtual void preview(raycast_info *rc, frame_data *frame) = 0;
    virtual void get_description(char *str) = 0;

    static tool *create_add_block_tool();
    static tool *create_remove_block_tool();
    static tool *create_remove_surface_tool();
    static tool *create_fire_projectile_tool(player *pl);
    static tool *create_remove_entity_tool();
    static tool *create_add_entity_tool();
    static tool *create_paint_surface_tool();
    static tool *create_add_room_tool();
};

