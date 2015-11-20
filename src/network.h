#pragma once

#include <enet/enet.h>
#include <stdint.h>
#include <glm/glm.hpp>

#include "ship_space.h"
#include "block.h"

enum class message_type : uint8_t {
    /* message types */
    server_msg = 0,
    ship_msg,
    update_msg,
};

enum class message_subtype_server : uint8_t {
    /* SERVER_MSG subtypes */
    server_vsn_msg = 0,
    client_vsn_msg,
    incompat_vsn_msg,
    slot_request,
    slot_granted,
    server_full,
    register_required,
    not_in_slot,
};

enum class message_subtype_ship : uint8_t {
    /* SHIP_MSG subtype */
    all_ship_request = 0,
    all_ship_reply,
    chunk_ship_reply,
};

enum class message_subtype_update : uint8_t {
    /* UPDATE_MSG subtype */
    set_block_type = 0,
    set_surface_type,
    remove_block,
    remove_surface,
};


/* assumes 4 byte int, obviously not always true */
/* FIXME: need a 4 byte integer vector */
#define unpack_static_int(x)            \
    (uint8_t)(((int)x & 0xFF000000) >> 6),   \
    (uint8_t)(((int)x & 0x00FF0000) >> 4),   \
    (uint8_t)(((int)x & 0x0000FF00) >> 2),   \
    (uint8_t)((int)x & 0x000000FF)

#define pack_int(arr, idx)      \
    ((((int)arr[idx]  ) << 6) | \
     (((int)arr[idx+1]) << 4) | \
     (((int)arr[idx+2]) << 2) | \
     (((int)arr[idx+3])))


/* version messages */
bool send_client_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch);
bool send_incompatible_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch);
bool send_server_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch);

/* slot messages */
bool request_slot(ENetPeer *peer);
bool send_register_required(ENetPeer *peer);
bool send_slots_full(ENetPeer *peer);
bool send_slot_granted(ENetPeer *peer);
bool send_not_in_slot(ENetPeer *peer);

/* ship messages */
bool request_whole_ship(ENetPeer *peer);
bool send_ship_chunk(ENetPeer *peer, glm::ivec3 ch, std::vector<unsigned char> *vbuf);
bool reply_whole_ship(ENetPeer *peer);

/* update messages */
bool set_block_type(ENetPeer *peer, glm::ivec3 block,
        enum block_type type);
bool set_block_surface(ENetPeer *peer, glm::ivec3 a, glm::ivec3 b,
    uint8_t idx, uint8_t st);

/* raw messages */
bool send_data(ENetPeer *peer, uint8_t *data, size_t size);
