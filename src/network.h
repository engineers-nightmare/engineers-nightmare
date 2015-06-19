#pragma once

#include <enet/enet.h>
#include <stdint.h>

/* message types */
#define SERVER_MSG 0x00

/* SERVER_MSG subtypes */
#define SERVER_VSN_MSG 0x00
#define CLIENT_VSN_MSG 0x01
#define INCOMPAT_VSN_MSG 0x02
#define SLOT_REQUEST 0x03
#define SLOT_GRANTED 0x04
#define SERVER_FULL 0x05
#define REGISTER_REQUIRED 0x06

bool send_client_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch);
bool send_incomptible_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch);
bool send_server_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch);
bool request_slot(ENetPeer *peer);
bool send_register_required(ENetPeer *peer);
bool send_slots_full(ENetPeer *peer);
bool send_slot_granted(ENetPeer *peer);
