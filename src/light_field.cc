#include "light_field.h"

#include <algorithm>

#include "block.h"
#include "component/component_system_manager.h"

light_field *light;

bool need_lightfield_update = false;

void
set_light_level(int x, int y, int z, int level) {
    if (x < 0 || x >= 128) return;
    if (y < 0 || y >= 128) return;
    if (z < 0 || z >= 128) return;

    int p = x + y * 128 + z * 128 * 128;
    if (level < 0) level = 0;
    if (level > 255) level = 255;

    light->data[p] = level;
}


unsigned char
get_light_level(int x, int y, int z) {
    if (x < 0 || x >= 128) return 0;
    if (y < 0 || y >= 128) return 0;
    if (z < 0 || z >= 128) return 0;

    return light->data[x + y * 128 + z * 128 * 128];
}


void
mark_lightfield_update(glm::ivec3 center) {
    glm::ivec3 half_extent = glm::ivec3(max_light_prop, max_light_prop, max_light_prop);
    if (need_lightfield_update) {
        lightfield_update_mins = center - half_extent;
        lightfield_update_maxs = center + half_extent;
    }
    else {
        lightfield_update_mins = glm::min(lightfield_update_mins,
            center - half_extent);
        lightfield_update_maxs = glm::max(lightfield_update_maxs,
            center + half_extent);
        need_lightfield_update = true;
    }
}


void
update_lightfield(ship_space *ship) {
    if (!need_lightfield_update) {
        /* nothing to do here */
        return;
    }

    /* TODO: opt for case where we're JUST adding light -- no need to clear & rebuild */
    /* This is general enough to cope with occluders & lights being added and removed. */

    /* 1. remove all existing light in the box */
    for (int k = lightfield_update_mins.z; k <= lightfield_update_maxs.z; k++)
        for (int j = lightfield_update_mins.y; j <= lightfield_update_maxs.y; j++)
            for (int i = lightfield_update_mins.x; i <= lightfield_update_maxs.x; i++)
                set_light_level(i, j, k, 0);

    /* 2. inject sources. the box is guaranteed to be big enough for max propagation
    * for all sources we'll add here. */
    for (auto i = 0u; i < light_man.buffer.num; i++) {
        auto ce = light_man.instance_pool.entity[i];
        auto pos = get_coord_containing(*pos_man.get_instance_data(ce).position);
        auto level = std::max(
            (int)get_light_level(pos.x, pos.y, pos.z),
            (int)(255 * light_man.instance_pool.intensity[i]));
        set_light_level(pos.x, pos.y, pos.z, level);
    }

    /* 3. propagate max_light_prop times. this is guaranteed to be enough to cover
    * the sources' area of influence. */
    for (int pass = 0; pass < max_light_prop; pass++) {
        for (int k = lightfield_update_mins.z; k <= lightfield_update_maxs.z; k++) {
            for (int j = lightfield_update_mins.y; j <= lightfield_update_maxs.y; j++) {
                for (int i = lightfield_update_mins.x; i <= lightfield_update_maxs.x; i++) {
                    int level = get_light_level(i, j, k);

                    block *b = ship->get_block(glm::ivec3(i, j, k));
                    if (!b)
                        continue;

                    if (light_permeable(b->surfs[surface_xm]))
                        level = std::max(level, get_light_level(i - 1, j, k) - light_atten);
                    if (light_permeable(b->surfs[surface_xp]))
                        level = std::max(level, get_light_level(i + 1, j, k) - light_atten);

                    if (light_permeable(b->surfs[surface_ym]))
                        level = std::max(level, get_light_level(i, j - 1, k) - light_atten);
                    if (light_permeable(b->surfs[surface_yp]))
                        level = std::max(level, get_light_level(i, j + 1, k) - light_atten);

                    if (light_permeable(b->surfs[surface_zm]))
                        level = std::max(level, get_light_level(i, j, k - 1) - light_atten);
                    if (light_permeable(b->surfs[surface_zp]))
                        level = std::max(level, get_light_level(i, j, k + 1) - light_atten);

                    set_light_level(i, j, k, level);
                }
            }
        }
    }

    /* All done. */
    light->upload();
    need_lightfield_update = false;
}
