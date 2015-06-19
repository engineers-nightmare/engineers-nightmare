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

void handle_server_message(ENetEvent *event, uint8_t *data)
{
    switch(*data) {
        case SERVER_VSN_MSG:
            printf("server version: %d.%d.%d\n", *(data + 1),
                    *(data + 2), *(data + 3));
            request_slot(event->peer);
            break;
        case INCOMPAT_VSN_MSG:
            printf("You must upgrade your client to at least %d.%d.%d\n",
                    *(data + 1), *(data + 2), *(data + 3));
            disconnect_peer(event->peer);
            break;
        case SLOT_GRANTED:
            printf("slot was granted!\n");
            break;
        case SERVER_FULL:
            printf("no free slots!\n");
            disconnect_peer(event->peer);
            break;
        case REGISTER_REQUIRED:
            printf("attempted to join before registering!\n");
            disconnect_peer(event->peer);
            break;
    }
}

void handle_message(ENetEvent *event)
{
    uint8_t *data;

    printf("[%x:%u] ", event->peer->address.host, event->peer->address.port);
    data = event->packet->data;
    switch(*data) {
        case SERVER_MSG:
            printf("server message(0x%02x): ", *(data + 1));
            handle_server_message(event, data + 1);
            break;
        default:
            printf("unknown message(0x%02x)\n", *data);
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

    client = enet_host_create(NULL, /* create a client host */
            1,          /* only allow 1 outgoing connection */
            2,          /* allow up 2 channels to be used, 0 and 1 */
            57600/8,    /* 56K modem with 56 Kbps downstream bandwidth */
            14400/8);   /* 56k modem with 14 Kbps upstream bandwidth */
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
                break;
            case ENET_EVENT_TYPE_CONNECT:
            case ENET_EVENT_TYPE_NONE:
                break;
            default:
                printf("timed out!\n");
        }
    }

    if(!disconnected)
        disconnect_peer(peer);

    enet_host_destroy(client);
    return EXIT_SUCCESS;
}
