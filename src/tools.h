#pragma once


struct raycast_info;


struct tool
{
    virtual void use(raycast_info *rc) = 0;
    virtual void preview(raycast_info *rc) = 0;
    virtual void get_description(char *str) = 0;

    static tool *create_add_block_tool();
};
