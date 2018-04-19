#include <string.h>

#include "../player.h"
#include "tools.h"
#include "../common.h"


struct fire_projectile_tool : tool
{
    player *pl;

    explicit fire_projectile_tool(player *pl) {
        this->pl = pl;
    }

    void use(ship_space *ship) override
    {
        pl->fire_projectile = true;
    }

    void get_description(char *str) override
    {
        strcpy(str, "Fire Projectile");
    }
};


tool *tool::create_fire_projectile_tool(player *pl) { return new fire_projectile_tool(pl); }
