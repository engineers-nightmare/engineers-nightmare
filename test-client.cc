#include <enet/enet.h>
#include "src/network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VSN_MAJOR 0
#define VSN_MINOR 1
#define VSN_PATCH 0

ENetPeer *peer;
ENetHost *client;
static bool disconnected;
ship_space *ship;

void disconnect_peer(ENetPeer *peer)
{
    ENetEvent event;

    enet_host_flush(client);

    enet_peer_disconnect(peer, 0);
    while(enet_host_service(client, &event, 3000) > 0) {
        switch(event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                disconnected = true;
                break;
            case ENET_EVENT_TYPE_CONNECT:
            case ENET_EVENT_TYPE_NONE:
                break;
        }
    }

    /* failed to disconnect in 3 seconds */
    if(!disconnected)
        enet_peer_reset(peer);
}

void handle_server_message(ENetEvent *event, uint8_t *data,
    message_subtype_server subtype)
{
    switch(subtype) {
        case message_subtype_server::server_vsn_msg:
        {
            uint8_t major = *(data + 0);
            uint8_t minor = *(data + 1);
            uint8_t patch = *(data + 2);

            printf("server version %d.%d.%d ", major, minor, patch);
            request_slot(event->peer);
            break;
        }
        case message_subtype_server::incompat_vsn_msg:
        {
            uint8_t major = *(data + 0);
            uint8_t minor = *(data + 1);
            uint8_t patch = *(data + 2);

            fprintf(stderr, "You must upgrade your client to at "
                "least v%d.%d.%d\n", major, minor, patch);
            disconnect_peer(event->peer);
            break;
        }
        case message_subtype_server::slot_granted:
            printf("slot was granted!\n");
            request_whole_ship(event->peer);
            break;
        case message_subtype_server::server_full:
            printf("no free slots!\n");
            disconnect_peer(event->peer);
            break;
        case message_subtype_server::register_required:
            printf("attempted to join before registering!\n");
            disconnect_peer(event->peer);
            break;
        case message_subtype_server::not_in_slot:
            printf("was not in a slot when requesting game information\n");
            break;
        default:
            break;
    }
}

void handle_ship_message(ENetEvent *event, uint8_t *data, message_subtype_ship subtype)
{
    switch(subtype) {
        case message_subtype_ship::all_ship_reply:
            printf("got ship info!\n");
            break;
        default:
            break;
    }
}

void handle_message(ENetEvent *event)
{
    uint8_t *data = event->packet->data;
    message_type type = (message_type)*data;

    printf("[%x:%u] ", event->peer->address.host, event->peer->address.port);

    switch(type) {
        case message_type::server_msg:
        {
            auto subtype = (message_subtype_server)*(data + 1);
            printf("server message(0x%02X): ", subtype);
            handle_server_message(event, data + 2, subtype);
            break;
        }
        case message_type::ship_msg:
        {
            auto subtype = (message_subtype_ship)*(data + 1);
            printf("ship message(0x%02X): ", subtype);
            handle_ship_message(event, data + 2, subtype);
            break;
        }
        default:
            printf("unknown message(0x%02x)\n", type);
    }
}

int do_connection(char *hostname, int port)
{
    ENetAddress addr;
    ENetEvent event;

    enet_address_set_host(&addr, hostname);
    addr.port = port;
    /* connect to remove host */
    peer = enet_host_connect(client, &addr, 2, 0);
    if(!peer) {
        fprintf(stderr, "Unable to connect to server\n");
        return 0;
    }

    /* 5 second timeout */
    if(enet_host_service(client, &event, 5000) > 0
            && event.type == ENET_EVENT_TYPE_CONNECT) {
        printf("connected to %s:%d\n", hostname, port);
        return 1;
    }

    enet_peer_reset(peer);
    printf("connection to %s:%d failed\n", hostname, port);
    return 0;
}

int main(int argc, char **argv)
{
    ENetEvent event;

    if(argc < 3) {
        fprintf(stderr, "requires a host and port!\n");
        return EXIT_FAILURE;
    }

    if(enet_initialize()) {
        fprintf(stderr, "failed to initialize enet!\n");
        return EXIT_FAILURE;
    }

    client = enet_host_create(nullptr, /* create a client host */
            1,                      /* only allow 1 outgoing connection */
            2,                      /* allow up 2 channels to be used, 0 and 1 */
            57600/8,                /* 56K modem with 56 Kbps downstream bandwidth */
            14400/8);               /* 56k modem with 14 Kbps upstream bandwidth */
    if(!client) {
        fprintf(stderr, "An error occured while creating an ENet client.\n");
        return EXIT_FAILURE;
    }

    if(!do_connection(argv[1], atoi(argv[2]))) {
        fprintf(stderr, "failed to connect to host\n");
        enet_host_destroy(client);
        return EXIT_FAILURE;
    }

    disconnected = false;
    send_client_version(peer, VSN_MAJOR, VSN_MINOR, VSN_PATCH);
    while(enet_host_service(client, &event, 1000) >= 0 && !disconnected) {
        switch(event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                handle_message(&event);
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("got disconnect\n");
                disconnected = true;
                break;
            case ENET_EVENT_TYPE_CONNECT:
            case ENET_EVENT_TYPE_NONE:
                break;
            default:
                disconnected = true;
                printf("timed out!\n");
        }
    }

    if(!disconnected)
        disconnect_peer(peer);

    enet_host_destroy(client);
    return EXIT_SUCCESS;
}
