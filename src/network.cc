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
send_ship_chunk(ENetPeer *peer, ship_space *space, int chunk_x, int chunk_y, int chunk_z)
{
    // Get data
    std::vector<unsigned char> *vbuf = space->serialize_chunk(chunk_x, chunk_y, chunk_z);
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
    packet->data[2+0] = (uint8_t)(chunk_x>>8);
    packet->data[2+1] = (uint8_t)(chunk_x);
    packet->data[2+2] = (uint8_t)(chunk_y>>8);
    packet->data[2+3] = (uint8_t)(chunk_y);
    packet->data[2+4] = (uint8_t)(chunk_z>>8);
    packet->data[2+5] = (uint8_t)(chunk_z);

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
        int chunk_x = c.first.x;
        int chunk_y = c.first.y;
        int chunk_z = c.first.z;

        if(!send_ship_chunk(peer, space, chunk_x, chunk_y, chunk_z)) {
            // FIXME: need to work out what to do if something fails
            return false;
        }
    }

    uint8_t data[2] = {SHIP_MSG, ALL_SHIP_REPLY};
    packet = enet_packet_create(data, 2, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool
set_block_type(ENetPeer *peer, glm::ivec3 b, enum block_type type)
{
    ENetPacket *packet;

    assert(peer);

    ship->get_block(b)->type = type;
    printf("set chunk at %.2f,%.2f,%.2f to %d\n", b.x, b.y, b.z, type);
    uint8_t data[15] = {UPDATE_MSG, SET_BLOCK_TYPE,
        unpack_static_int(b.x),
        unpack_static_int(b.y),
        unpack_static_int(b.z),
        type
    };
    packet = enet_packet_create(data, sizeof(data), ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool
set_block_surface(ENetPeer *peer, glm::ivec3 b, glm::vec3 os, uint8_t idx,
        uint8_t st)
{
    ENetPacket *packet;

    assert(peer);

    ship->get_block(b)->surfs[idx] = (enum surface_type)st;
    ship->get_block(os)->surfs[idx ^ 1] = (enum surface_type)st;

    printf("set texture at %.2f,%.2f,%.2f|%.2f,%.2f,%.2f to %d on %d\n",
            b.x, b.y, b.z, os.x, os.y, os.z, st, idx);
    uint8_t data[28] = {UPDATE_MSG, SET_SURFACE_TYPE,
        unpack_static_int(b.x),
        unpack_static_int(b.y),
        unpack_static_int(b.z),
        unpack_static_int(os.x),
        unpack_static_int(os.y),
        unpack_static_int(os.z),
        idx, st
    };
    packet = enet_packet_create(data, sizeof(data), ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}


bool
remove_surface(ENetPeer *peer, glm::vec3 b, glm::vec3 os, uint8_t idx)
{
    ENetPacket *packet;

    assert(peer);

    ship->get_block(b)->surfs[idx] = surface_none;
    ship->get_block(os)->surfs[idx ^ 1] =surface_none;
    // remove_ents_from_surface(b.x, b.y, b.z, idx);
    // remove_ents_from_surface(os.x, os.y, os.z, idx ^ 1);

    uint8_t data[27] = {UPDATE_MSG, REMOVE_SURFACE,
        unpack_static_int(b.x),
        unpack_static_int(b.y),
        unpack_static_int(b.z),
        unpack_static_int(os.x),
        unpack_static_int(os.y),
        unpack_static_int(os.z),
        idx
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
