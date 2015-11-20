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

static bool send_version_message(ENetPeer *peer, message_subtype_server subtype,
        uint8_t major, uint8_t minor, uint8_t patch)
{
    assert(peer);

    uint8_t data[5] = {(uint8_t)message_type::server_msg,
        (uint8_t)subtype, major, minor, patch };
    auto packet = enet_packet_create(data, 5, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool send_client_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch)
{
    return send_version_message(peer, message_subtype_server::client_vsn_msg,
        major, minor, patch);
}

bool send_server_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch)
{
    return send_version_message(peer, message_subtype_server::server_vsn_msg,
        major, minor, patch);
}

bool send_incompatible_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch)
{
    return send_version_message(peer,message_subtype_server::incompat_vsn_msg,
        major, minor, patch);
}

bool basic_server_message(ENetPeer *peer, message_subtype_server subtype)
{
    assert(peer);

    uint8_t data[2] = {(uint8_t)message_type::server_msg, (uint8_t)subtype};
    auto packet = enet_packet_create(data, 2, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool request_slot(ENetPeer *peer)
{
    return basic_server_message(peer, message_subtype_server::slot_request);
}

bool send_register_required(ENetPeer *peer)
{
    return basic_server_message(peer, message_subtype_server::register_required);
}

bool send_slots_full(ENetPeer *peer)
{
    return basic_server_message(peer, message_subtype_server::server_full);
}

bool send_slot_granted(ENetPeer *peer)
{
    return basic_server_message(peer, message_subtype_server::slot_granted);
}

bool send_not_in_slot(ENetPeer *peer)
{
    return basic_server_message(peer, message_subtype_server::not_in_slot);
}

bool basic_ship_message(ENetPeer *peer, message_subtype_ship subtype)
{
    assert(peer);

    uint8_t data[2] = { (uint8_t)message_type::ship_msg, (uint8_t)subtype};
    auto packet = enet_packet_create(data, 2, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool request_whole_ship(ENetPeer *peer) {
    return basic_ship_message(peer, message_subtype_ship::all_ship_request);
}

bool
send_ship_chunk(ENetPeer *peer, glm::ivec3 ch, std::vector<unsigned char> *vbuf)
{
    // Get data
    if (!vbuf) {
        return false;
    }

    // Allocate packet
    ENetPacket *packet =
        enet_packet_create(nullptr, vbuf->size() + 8, ENET_PACKET_FLAG_RELIABLE);
    assert( packet );

    // Add in standard header
    packet->data[0] = (uint8_t)message_type::ship_msg;
    packet->data[1] = (uint8_t)message_subtype_ship::chunk_ship_reply;

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

    // Send and return!
    return send_packet(peer, packet);
}

bool reply_whole_ship(ENetPeer *peer) {
    ENetPacket *packet;
    assert(peer);

    uint8_t data[2] = { (uint8_t)message_type::ship_msg,
        (uint8_t)message_subtype_ship::all_ship_reply};
    packet = enet_packet_create(data, 2, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool
set_block_type(ENetPeer *peer, glm::ivec3 block, enum block_type type)
{
    ENetPacket *packet;

    assert(peer);

    printf("set chunk at %d,%d,%d to %d\n", block.x, block.y, block.z, type);
    uint8_t data[15] = { (uint8_t)message_type::update_msg,
        (uint8_t)message_subtype_update::set_block_type,
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
    uint8_t data[28] = {(uint8_t)message_type::update_msg,
        (uint8_t)message_subtype_update::set_surface_type,
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
