#include "network.h"

#include <cassert>
#include <stdio.h>

#include "ship_space.h"
#include "packet_writer.h"

extern ship_space *ship;

extern void
remove_ents_from_surface(int x, int y, int z, int face);

static void destroy_packet_callback(ENetPacket *packet) {
    delete packet->data;
}

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

    uint8_t data[5];
    packet_writer pw(data, 5);

    pw.put(message_type::server);
    pw.put(subtype);
    pw.put(major);
    pw.put(minor);
    pw.put(patch);

    auto packet = enet_packet_create(pw.buf, pw.len, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool send_client_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch)
{
    return send_version_message(peer, message_subtype_server::client_version,
        major, minor, patch);
}

bool send_server_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch)
{
    return send_version_message(peer, message_subtype_server::server_version,
        major, minor, patch);
}

bool send_incompatible_version(ENetPeer *peer, uint8_t major, uint8_t minor,
        uint8_t patch)
{
    return send_version_message(peer,message_subtype_server::incompatible_version,
        major, minor, patch);
}

bool basic_server_message(ENetPeer *peer, message_subtype_server subtype)
{
    assert(peer);

    uint8_t data[2];
    packet_writer pw(data, 2);

    pw.put(message_type::server);
    pw.put(subtype);

    auto packet = enet_packet_create(pw.buf, pw.len, ENET_PACKET_FLAG_RELIABLE);
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

    uint8_t buf[2];
    packet_writer pw(&buf[0], 2);
    pw.put(message_type::ship);
    pw.put(subtype);

    auto packet = enet_packet_create(pw.buf, pw.len, ENET_PACKET_FLAG_RELIABLE);
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
    auto *buffer = new uint8_t[4 * 1024];
    packet_writer pw(buffer, 4 * 1024);

    pw.put(message_type::ship);
    pw.put(message_subtype_ship::chunk_ship_reply);
    pw.put(ch);

    for (auto d : *vbuf) {
        pw.put(d);
    }
    
    ENetPacket *packet =
        enet_packet_create(pw.buf, pw.len, ENET_PACKET_FLAG_RELIABLE | ENET_PACKET_FLAG_NO_ALLOCATE);
    packet->freeCallback = destroy_packet_callback;
    assert(packet);

    // Send and return!
    return send_packet(peer, packet);
}

bool reply_whole_ship(ENetPeer *peer) {
    assert(peer);

    uint8_t data[2];
    packet_writer pw(data, 2);

    pw.put(message_type::ship);
    pw.put(message_subtype_ship::all_ship_reply);

    auto packet = enet_packet_create(pw.buf, pw.len, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool
set_block_type(ENetPeer *peer, glm::ivec3 block, enum block_type type)
{
    assert(peer);

    printf("set block at %d,%d,%d to %d\n", block.x, block.y, block.z, type);

    uint8_t data[15];
    packet_writer pw(data, 15);

    pw.put(message_type::update);
    pw.put(message_subtype_update::set_block_type);
    pw.put(block);
    pw.put(type);

    auto packet = enet_packet_create(pw.buf, pw.len, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool
set_block_surface(ENetPeer *peer, glm::ivec3 a, glm::ivec3 b,
        uint8_t idx, uint8_t st)
{
    assert(peer);

    printf("set texture at %d,%d,%d|%d,%d,%d to %d on %d\n",
            a.x, a.y, a.z, b.x, b.y, b.z, st, idx);
    
    uint8_t data[28];
    packet_writer pw(data, 28);

    pw.put(message_type::update);
    pw.put(message_subtype_update::set_surface_type);
    pw.put(a);
    pw.put(b);
    pw.put(idx);
    pw.put(st);

    auto packet = enet_packet_create(pw.buf, pw.len, ENET_PACKET_FLAG_RELIABLE);
    return send_packet(peer, packet);
}

bool
send_data(ENetPeer *peer, uint8_t *data, size_t size)
{
    assert(peer && data);
    return send_packet(peer, enet_packet_create(data, size,
                    ENET_PACKET_FLAG_RELIABLE));

}
