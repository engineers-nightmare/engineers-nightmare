#pragma once

#include "../block.h"

struct player;
struct raycast_info;
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
    static tool *create_remove_surface_entity_tool();
    static tool *create_add_block_entity_tool();
    static tool *create_add_surface_entity_tool();
};

struct add_room_tool : tool {
    unsigned room_size = 0;

    bool can_use(const raycast_info *rc);

    void use(raycast_info *rc) override;

    void alt_use(raycast_info *rc) override;

    void long_use(raycast_info *rc) override;

    void cycle_mode() override;

    void preview(raycast_info *rc, frame_data *frame) override;

    void get_description(char *str) override;
};

struct paint_surface_tool : tool
{
    surface_type select_type;
    surface_type replace_type;

    glm::ivec3 start_block;
    surface_index start_index;

    enum class replace_mode {
        all,
        match,
    } mode = replace_mode::match;

    enum class paint_state {
        started,
        idle,
    } state = paint_state::idle;

    paint_surface_tool() : select_type(surface_wall), replace_type(surface_wall) {}

    bool can_use(const raycast_info *rc) const;

    void use(raycast_info *rc) override;

    void finish_paint(glm::ivec3 end);

    void alt_use(raycast_info *rc) override;

    void long_use(raycast_info *rc) override;

    void cycle_mode() override;

    void preview(raycast_info *rc, frame_data *frame) override;

    void get_description(char *str) override;

    void select() override;
    void unselect() override;
};
