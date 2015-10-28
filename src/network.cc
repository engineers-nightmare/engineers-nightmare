#include "network.h"

#include <cassert>
#include <stdio.h>

#include "ship_space.h"

extern ship_space *ship;

extern void
remove_ents_from_surface(int x, int y, int z, int face);

static bool send_packet(ENetPeer *peer, ENetPacket *packet)
{
    if(!packet) {
        fprintf(stderr, "failed to create packet!\n");
        return false;
    }

    if(enet_peer_send(peer, 0, packet) < 0) {
        fprintf(stderr, "failed to send packet!\n");
        return false;
    }

    return true;
}

static bool send_version_message(ENetPeer *peer, uint8_t subtype,
        uint8_t major, uint8_t minor, uint8_t patch)
{
    ENetPacket *packet;

    assert(peer);

    uint8_t data[5] = { SERVER_MSG, subtype, major, minor, patch };
    packet = enet_packet_create(data, 5, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool send_client_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch)
{
    return send_version_message(peer, CLIENT_VSN_MSG, major, minor, patch);
}

bool send_server_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch)
{
    return send_version_message(peer, SERVER_VSN_MSG, major, minor, patch);
}

bool send_incompatible_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch)
{
    return send_version_message(peer, INCOMPAT_VSN_MSG, major, minor, patch);
}

bool basic_server_message(ENetPeer *peer, uint8_t subtype)
{
    ENetPacket *packet;

    assert(peer);

    uint8_t data[2] = {SERVER_MSG, subtype};
    packet = enet_packet_create(data, 2, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool request_slot(ENetPeer *peer)
{
    return basic_server_message(peer, SLOT_REQUEST);
}

bool send_register_required(ENetPeer *peer)
{
    return basic_server_message(peer, REGISTER_REQUIRED);
}

bool send_slots_full(ENetPeer *peer)
{
    return basic_server_message(peer, SERVER_FULL);
}

bool send_slot_granted(ENetPeer *peer)
{
    return basic_server_message(peer, SLOT_GRANTED);
}

bool send_not_in_slot(ENetPeer *peer)
{
    return basic_server_message(peer, NOT_IN_SLOT);
}

bool basic_ship_message(ENetPeer *peer, uint8_t subtype)
{
    ENetPacket *packet;

    assert(peer);

    uint8_t data[2] = {SHIP_MSG, subtype};
    packet = enet_packet_create(data, 2, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool request_whole_ship(ENetPeer *peer) {
    return basic_ship_message(peer, ALL_SHIP_REQUEST);
}

bool
send_ship_chunk(ENetPeer *peer, ship_space *space, glm::ivec3 ch)
{
    // Get data
    std::vector<unsigned char> *vbuf = space->serialize_chunk(ch);
    if (!vbuf) {
        return false;
    }

    // Allocate packet
    ENetPacket *packet = enet_packet_create(NULL, vbuf->size() + 8, ENET_PACKET_FLAG_RELIABLE);
    assert( packet );

    // Add in standard header
    packet->data[0] = SHIP_MSG;
    packet->data[1] = CHUNK_SHIP_REPLY;

    // Add in chunk coordinates
    // TODO: proper endian-safe integer pack/unpack
    packet->data[2+0] = (uint8_t)(ch.x>>8);
    packet->data[2+1] = (uint8_t)(ch.x);
    packet->data[2+2] = (uint8_t)(ch.y>>8);
    packet->data[2+3] = (uint8_t)(ch.y);
    packet->data[2+4] = (uint8_t)(ch.z>>8);
    packet->data[2+5] = (uint8_t)(ch.z);

    // Add in data
    memcpy(packet->data + 8, vbuf->data(), vbuf->size());
    delete vbuf;

    // Send and return!
    return send_packet(peer, packet);
}

bool reply_whole_ship(ENetPeer *peer, ship_space *space) {
    ENetPacket *packet;

    assert(peer && space);

    // TODO: send more than one chunk
    for (auto c : space->chunks) {
        glm::ivec3 chunk;
        chunk.x = c.first.x;
        chunk.y = c.first.y;
        chunk.z = c.first.z;

        if(!send_ship_chunk(peer, space, chunk)) {
            // FIXME: need to work out what to do if something fails
            return false;
        }
    }

    uint8_t data[2] = {SHIP_MSG, ALL_SHIP_REPLY};
    packet = enet_packet_create(data, 2, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool
set_block_type(ENetPeer *peer, glm::ivec3 block, enum block_type type)
{
    ENetPacket *packet;

    assert(peer);

    printf("set chunk at %d,%d,%d to %d\n", block.x, block.y, block.z, type);
    uint8_t data[15] = {UPDATE_MSG, SET_BLOCK_TYPE,
        unpack_static_int(block.x),
        unpack_static_int(block.y),
        unpack_static_int(block.z),
        type
    };
    packet = enet_packet_create(data, sizeof(data), ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool
set_block_surface(ENetPeer *peer, glm::ivec3 a, glm::ivec3 b,
        uint8_t idx, uint8_t st)
{
    ENetPacket *packet;

    assert(peer);

    printf("set texture at %d,%d,%d|%d,%d,%d to %d on %d\n",
            a.x, a.y, a.z, b.x, b.y, b.z, st, idx);
    uint8_t data[28] = {UPDATE_MSG, SET_SURFACE_TYPE,
        unpack_static_int(a.x),
        unpack_static_int(a.y),
        unpack_static_int(a.z),
        unpack_static_int(b.x),
        unpack_static_int(b.y),
        unpack_static_int(b.z),
        idx, st
    };
    packet = enet_packet_create(data, sizeof(data), ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool
send_data(ENetPeer *peer, uint8_t *data, size_t size)
{
    assert(peer && data);
    return send_packet(peer, enet_packet_create(data, size,
                    ENET_PACKET_FLAG_RELIABLE));

}
