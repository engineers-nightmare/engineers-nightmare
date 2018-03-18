#include <cstdio>
#include <cstdint>
#include <iostream>
#include <iomanip>

#include "ship_space.h"

class loader {
    FILE *f;

public:
    explicit loader(char const *filename) {
        f = fopen(filename, "rb");
    }

    ~loader() {
        fclose(f);
    }

    template<typename T> T read(uint32_t &amt_read) = delete;

    uint32_t read_type(uint32_t &amt_read);

    uint32_t read_length(uint32_t &amt_read);

    void skip(uint32_t i);
};


template<>
uint32_t loader::read<uint32_t>(uint32_t &amt_read) {
    uint32_t x;
    auto sz = sizeof(x);
    amt_read = (uint32_t)(fread(&x, sz, 1, f) * sz);
    return x;
}

template<>
int32_t loader::read<int32_t>(uint32_t &amt_read) {
    int32_t x;
    auto sz = sizeof(x);
    amt_read = (uint32_t)(fread(&x, sz, 1, f) * sz);
    return x;
}

template<>
unsigned char loader::read<unsigned char>(uint32_t &amt_read) {
    unsigned char x;
    auto sz = sizeof(x);
    amt_read = (uint32_t)(fread(&x, sz, 1, f) * sz);
    return x;
}

template<>
float loader::read<float>(uint32_t &amt_read) {
    float x;
    auto sz = sizeof(x);
    amt_read = (uint32_t)(fread(&x, sz, 1, f) * sz);
    return x;
}

uint32_t loader::read_type(uint32_t &amt_read) {
    uint32_t type = read<uint32_t>(amt_read);
    return type;
}

uint32_t loader::read_length(uint32_t &amt_read) {
    uint32_t length = read<uint32_t>(amt_read);
    return length;
}

void loader::skip(uint32_t i) {
    fseek(f, i, SEEK_CUR);
}

static uint32_t load_chunk(loader *l, ship_space *ship) {
    auto ch = new chunk();

    uint32_t read = 0;
    auto chunk_size = l->read_length(read);
    chunk_size += read;
    uint32_t chunk_read = read;
    glm::ivec3 chunk_pos;

    while (chunk_read < chunk_size) {
        auto type = l->read_type(read);
        chunk_read += read;
        auto size = l->read_length(read);
        chunk_read += read;
        if (type == fourcc("INFO")) {
            assert(size == sizeof(uint32_t) * 3);

            chunk_pos.x = l->read<uint32_t>(read);
            chunk_read += read;

            chunk_pos.y = l->read<uint32_t>(read);
            chunk_read += read;

            chunk_pos.z = l->read<uint32_t>(read);
            chunk_read += read;
        } else if (type == fourcc("FRAM")) {
            assert(size == sizeof(unsigned char) * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);

            for (auto k = 0u; k < CHUNK_SIZE; k++) {
                for (auto j = 0u; j < CHUNK_SIZE; j++) {
                    for (auto i = 0u; i < CHUNK_SIZE; i++) {
                        ch->blocks.get(i, j, k);
                        auto bl = ch->blocks.get(i, j, k);
                        bl->type = (block_type)l->read<unsigned char>(read);
                        chunk_read += read;
                    }
                }
            }
        } else if (type == fourcc("SURF")) {
            assert(size == sizeof(unsigned char) * face_count * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);

            for (auto k = 0u; k < CHUNK_SIZE; k++) {
                for (auto j = 0u; j < CHUNK_SIZE; j++) {
                    for (auto i = 0u; i < CHUNK_SIZE; i++) {
                        ch->blocks.get(i, j, k);
                        auto bl = ch->blocks.get(i, j, k);
                        for (auto &surf : bl->surfs) {
                            surf = (surface_type)l->read<unsigned char>(read);
                            chunk_read += read;
                        }
                    }
                }
            }
        } else if (type == fourcc("WIRE")) {
            assert(size == sizeof(unsigned char) * face_count * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);

            for (auto k = 0u; k < CHUNK_SIZE; k++) {
                for (auto j = 0u; j < CHUNK_SIZE; j++) {
                    for (auto i = 0u; i < CHUNK_SIZE; i++) {
                        ch->blocks.get(i, j, k);
                        auto bl = ch->blocks.get(i, j, k);
                        for (auto &has_wire : bl->wire[0].has_wire) {
                            has_wire = (bool)l->read<unsigned char>(read);
                            chunk_read += read;
                        }
                    }
                }
            }
        } else if (type == fourcc("CONN")) {
            assert(size == sizeof(uint32_t) * face_count * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);

            for (auto k = 0u; k < CHUNK_SIZE; k++) {
                for (auto j = 0u; j < CHUNK_SIZE; j++) {
                    for (auto i = 0u; i < CHUNK_SIZE; i++) {
                        ch->blocks.get(i, j, k);
                        auto bl = ch->blocks.get(i, j, k);
                        for (auto &wire_bit : bl->wire[0].wire_bits) {
                            wire_bit = l->read<uint32_t>(read);
                            chunk_read += read;
                        }
                    }
                }
            }
        }
    }
    ch->dirty();
    ship->chunks.insert({chunk_pos, ch});
    ship->mins = glm::min(ship->mins, chunk_pos);
    ship->maxs = glm::max(ship->maxs, chunk_pos);

    return chunk_read;
}

static uint32_t load_zone(loader *l, std::vector<std::pair<glm::ivec3, zone_info>> &zones) {
    uint32_t read = 0;
    uint32_t zone_read = 0;
    uint32_t zone_length = l->read_length(zone_read);
    zone_length += zone_read;

    assert(zone_length == 3 * sizeof(uint32_t) + sizeof(float) + zone_read);

    glm::ivec3 pos;
    pos.x = l->read<uint32_t>(read);
    zone_read += read;

    pos.y = l->read<uint32_t>(read);
    zone_read += read;

    pos.z = l->read<uint32_t>(read);
    zone_read += read;

    auto air = l->read<float>(read);
    zone_read += read;

    zones.push_back({pos, {air}});

    return zone_read;
}

static void load_ship(loader *l, ship_space *ship) {
    uint32_t read = 0;
    auto ship_size = l->read_length(read);
    auto ship_read = 0u;
    assert(ship_size > 0);

    std::vector<std::pair<glm::ivec3, zone_info>> zones;

    while (ship_read < ship_size) {
        auto type = l->read_type(read);
        ship_read += read;
        if (type == fourcc("CHNK")) {
            ship_read += load_chunk(l, ship);
        } else if (type == fourcc("ZONE")) {
            ship_read += load_zone(l, zones);
        } else {
            // unknown, skip this
            std::cout << "***** Ship loader encountered unknown type: 0x" << std::setfill('0') << std::setw(8)
                      << std::hex << type << " *****" << std::endl;

            auto size = l->read_length(read);
            l->skip(size);
            ship_read += read;
            ship_read += size;
        }
    }

    ship->rebuild_topology();
    for (auto zone : zones) {
        topo_info *t = topo_find(ship->get_topo_info(zone.first));
        zone_info *z = ship->get_zone_info(t);
        if (!z) {
            /* if there wasn't a zone, make one */
            ship->zones[t] = new zone_info(zone.second);
        }
        else {
            // weird if we're here?
            assert(false);
        }
    }
}

void load(ship_space *ship, char const *filename) {
    loader l(filename);

    auto read = 0u;
    auto type = l.read_type(read);
    assert(type == fourcc("SHIP"));
    load_ship(&l, ship);
    assert(ship->validate());
}