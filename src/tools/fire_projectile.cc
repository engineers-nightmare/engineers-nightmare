#include "../tools.h"

#include "../player.h"

#include <string.h>


struct fire_projectile_tool : tool
{
    player *pl;

    explicit fire_projectile_tool(player *pl) {
        this->pl = pl;
    }

    void use(raycast_info *rc) override
    {
        pl->fire_projectile = true;
    }

    void alt_use(raycast_info *rc) override {}

    void long_use(raycast_info *rc) override {}

    void cycle_mode() override {}

    void preview(raycast_info *rc) override {}

    void get_description(char *str) override
    {
        strcpy(str, "Fire Projectile");
    }
};


tool *tool::create_fire_projectile_tool(player *pl) { return new fire_projectile_tool(pl); }
