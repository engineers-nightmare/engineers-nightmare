#pragma once

#include <enet/enet.h>
#include <stdint.h>

#include "ship_space.h"

/* message types */
#define SERVER_MSG          0x00
#define SHIP_MSG            0x01

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
bool reply_whole_ship(ENetPeer *peer, ship_space *space);
