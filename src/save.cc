#include <cstdio>
#include <cstdint>

#include "ship_space.h"

class saver {
    FILE * f;

public:
    explicit saver(char const *filename) {
        f = fopen(filename, "wb");
    }

    ~saver() {
        fclose(f);
    }

    template<typename T> void write(T t) = delete;

    long begin_lump(uint32_t type);
    void end_lump(long start);
};

template<>
void saver::write<uint32_t>(uint32_t x) {
    fwrite(&x, 1, sizeof(x), f);
}

template<>
void saver::write<int32_t>(int32_t x) {
    fwrite(&x, 1, sizeof(x), f);
}

template<>
void saver::write<unsigned char>(unsigned char x) {
    fwrite(&x, 1, sizeof(x), f);
}

template<>
void saver::write<float>(float x) {
    fwrite(&x, 1, sizeof(x), f);
}

long saver::begin_lump(uint32_t type) {
    write(type);
    write(0u);
    return ftell(f);
}

void saver::end_lump(long start) {
    // move back and patch in the length
    long pos = ftell(f);
    fseek(f, start - 4, SEEK_SET);
    write<uint32_t>(pos - start);
    fseek(f, pos, SEEK_SET);
}

static void save_chunk(saver *s, std::pair<glm::ivec3, chunk *> chunk) {
    auto chunk_lump = s->begin_lump('CHNK');

    auto info_lump = s->begin_lump('INFO');
    s->write(chunk.first.x);
    s->write(chunk.first.y);
    s->write(chunk.first.z);
    s->end_lump(info_lump);

    auto frames_lump = s->begin_lump('FRAM');
    for (auto k = 0u; k < CHUNK_SIZE; k++) {
        for (auto j = 0u; j < CHUNK_SIZE; j++) {
            for (auto i = 0u; i < CHUNK_SIZE; i++) {
                auto bl = chunk.second->blocks.get(i, j, k);
                s->write<unsigned char>(bl->type);
            }
        }
    }
    s->end_lump(frames_lump);

    auto surfs_lump = s->begin_lump('SURF');
    for (auto k = 0u; k < CHUNK_SIZE; k++) {
        for (auto j = 0u; j < CHUNK_SIZE; j++) {
            for (auto i = 0u; i < CHUNK_SIZE; i++) {
                auto bl = chunk.second->blocks.get(i, j, k);
                for (auto &surf : bl->surfs) {
                    s->write<unsigned char>(surf);
                }
            }
        }
    }
    s->end_lump(surfs_lump);

    auto wire_lump = s->begin_lump('WIRE');
    for (auto k = 0u; k < CHUNK_SIZE; k++) {
        for (auto j = 0u; j < CHUNK_SIZE; j++) {
            for (auto i = 0u; i < CHUNK_SIZE; i++) {
                auto bl = chunk.second->blocks.get(i, j, k);
                for (bool f : bl->has_wire) {
                    s->write<unsigned char>(f);
                }
            }
        }
    }
    s->end_lump(wire_lump);

    auto conn_lump = s->begin_lump('CONN');
    for (auto k = 0u; k < CHUNK_SIZE; k++) {
        for (auto j = 0u; j < CHUNK_SIZE; j++) {
            for (auto i = 0u; i < CHUNK_SIZE; i++) {
                auto bl = chunk.second->blocks.get(i, j, k);
                for (unsigned int wire_bit : bl->wire_bits) {
                    s->write<uint32_t>(wire_bit);
                }
            }
        }
    }
    s->end_lump(conn_lump);

    s->end_lump(chunk_lump);
}

static void save_zone(saver *s, ship_space *ship, std::pair<topo_info *, zone_info *> zone) {
    glm::ivec3 pos;
    if (!ship->topo_to_pos(zone.first, &pos))
        return; /* not a real zone. this was probably outside. */

    auto zone_lump = s->begin_lump('ZONE');

    s->write(pos.x);
    s->write(pos.y);
    s->write(pos.z);

    s->write(zone.second->air_amount);

    s->end_lump(zone_lump);
}

static void save_ship(saver *s, ship_space *ship) {
    auto ship_lump = s->begin_lump('SHIP');
    for (auto chunk : ship->chunks) {
        save_chunk(s, chunk);
    }
    for (auto zone : ship->zones) {
        save_zone(s, ship, zone);
    }
    s->end_lump(ship_lump);
}

void save(ship_space *ship, char const *filename) {
    saver s(filename);
    save_ship(&s, ship);
}