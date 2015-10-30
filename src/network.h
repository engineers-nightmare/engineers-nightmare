#pragma once

#include <enet/enet.h>
#include <stdint.h>
#include <glm/glm.hpp>

#include "ship_space.h"
#include "block.h"

/* message types */
#define SERVER_MSG          0x00
#define SHIP_MSG            0x01
#define UPDATE_MSG          0x02

/* SERVER_MSG subtypes */
#define SERVER_VSN_MSG      0x00
#define CLIENT_VSN_MSG      0x01
#define INCOMPAT_VSN_MSG    0x02
#define SLOT_REQUEST        0x03
#define SLOT_GRANTED        0x04
#define SERVER_FULL         0x05
#define REGISTER_REQUIRED   0x06
#define NOT_IN_SLOT         0x07

/* SHIP_MSG subtype */
#define ALL_SHIP_REQUEST    0x00
#define ALL_SHIP_REPLY      0x01
#define CHUNK_SHIP_REPLY    0x02

/* UPDATE_MSG subtype */
#define SET_BLOCK_TYPE      0x00
#define SET_SURFACE_TYPE    0x01
#define REMOVE_BLOCK        0x02
#define REMOVE_SURFACE      0x03


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
bool send_ship_chunk(ENetPeer *peer, ship_space *space, glm::ivec3 ch);
bool reply_whole_ship(ENetPeer *peer, ship_space *space);

/* update messages */
bool set_block_type(ENetPeer *peer, glm::ivec3 block,
        enum block_type type);
bool set_block_surface(ENetPeer *peer, glm::ivec3 a, glm::ivec3 b,
    uint8_t idx, uint8_t st);

/* raw messages */
bool send_data(ENetPeer *peer, uint8_t *data, size_t size);
