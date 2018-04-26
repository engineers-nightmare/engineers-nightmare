#pragma once

#include "../block.h"
#include "../common.h"

struct player;
struct frame_data;
struct ship_space;


struct tool
{
    virtual ~tool() = default;

    virtual void pre_use(player *pl) {}

    virtual void use() {}
    virtual void alt_use() {}
    virtual void long_use() {}
    virtual void long_alt_use() {}
    virtual void select() {}
    virtual void unselect() {}

    virtual void cycle_mode() {}

    virtual void do_offscreen_render() {}
    virtual void preview(frame_data *frame) {}
    virtual void get_description(char *str) = 0;
    virtual void update() {};

    static tool *create_remove_block_tool();
    static tool *create_remove_surface_tool();
    static tool *create_fire_projectile_tool(player *pl);
    static tool *create_remove_entity_tool();
    static tool *create_add_entity_tool();
    static tool *create_paint_surface_tool();
    static tool *create_cut_wall_tool();
    static tool *create_wiring_tool();
    static tool *create_add_shaped_block_tool();
    static tool *create_customize_entity_tool();
    static tool *create_tether_tool();
};

