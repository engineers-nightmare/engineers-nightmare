#include <enet/enet.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/network.h"
#include "src/ship_space.h"

#define MAX_SLOTS 5

#define VSN_MAJOR 0
#define VSN_MINOR 1
#define VSN_PATCH 0
#define MIN_CLIENT_MAJOR 0
#define MIN_CLIENT_MINOR 1
#define MIN_CLIENT_PATCH 0
#define MAKE_VERSION_CHECK(ma, mi, pa) \
    (ma *  1000000 + mi * 1000 + pa)
#define CHECK_VERSION(vma, vmi, vpa) \
    (MAKE_VERSION_CHECK(vma, vmi, vpa) >= \
     MAKE_VERSION_CHECK(MIN_CLIENT_MAJOR, MIN_CLIENT_MINOR, \
         MIN_CLIENT_PATCH))

static ENetPeer *peers[MAX_SLOTS];
static ship_space *ship;

struct peer_info {
    unsigned vsn_str;
};

enum msg_ret_type {
    MSG_OK,
    MSG_PEER_DISCONNECT,
    MSG_UNKNOWN,
};

static ENetHost *server;

int init(void)
{
    ship = ship_space::mock_ship_space();
    if(!ship) {
        fprintf(stderr, "failed to load mock ship space\n");
        return 0;
    }

    return 1;
}

/* handle the submessage of a SERVER_MSG */
void handle_server_message(ENetEvent *event, uint8_t *data)
{
    struct peer_info *pi;
    int i;

    switch(*data) {
        case CLIENT_VSN_MSG:
            printf("client version %d.%d.%d ", *(data + 1),
                    *(data + 2), *(data + 3));
            if(CHECK_VERSION(*(data + 1), *(data + 2), *(data + 3))) {
                printf("compatible\n");
                send_server_version(event->peer, VSN_MAJOR, VSN_MINOR,
                        VSN_PATCH);

                pi = new struct peer_info;
                pi->vsn_str = MAKE_VERSION_CHECK(*(data + 1),
                        *(data + 2), *(data + 3));
                event->peer->data = pi;
            }else {
                printf("incompatible!\n");
                send_incomptible_version(event->peer, MIN_CLIENT_MAJOR,
                        MIN_CLIENT_MINOR, MIN_CLIENT_PATCH);
            }
            break;
        case SLOT_REQUEST:
            if(!event->peer->data)
                send_register_required(event->peer);
            for(i = 0; i < MAX_SLOTS; i++)
                if(!peers[i]) {
                    peers[i] = event->peer;
                    break;
                }

            if(i == MAX_SLOTS) {
                send_slots_full(event->peer);
            } else {
                send_slot_granted(event->peer);
            }

            break;
    }
}

/*  handle any incoming messages */
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

/* handle a remote disconnect. remove all references to the cliet */
void handle_disconnect(ENetEvent *event)
{
    int i;

    printf("%x:%u disconnected\n", event->peer->address.host,
            event->peer->address.port);
    if(event->peer->data){
        for(i = 0; i < MAX_SLOTS; i++) {
            if(peers[i] && peers[i]->connectID == event->peer->connectID) {
                peers[i] = NULL;
                break;
            }
        }
        delete (struct peer_info*)event->peer->data;
    }
}

/* listen/dispatch loop */
int server_loop()
{
    ENetEvent event;
    int server_ret;

    do {
        while((server_ret = enet_host_service(server, &event, 5000)) > 0) {
            switch(event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("A new client connected from %x:%u.\n",
                            event.peer->address.host,
                            event.peer->address.port);
                    event.peer->data = NULL;
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    handle_disconnect(&event);
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    handle_message(&event);
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_NONE:
                    break;
            }
        }
        if(server_ret == 0)
            printf("timed out!\n");
    } while(server_ret >= 0);

    if(server_ret < 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    int ret;
    ENetAddress addr;

    if(argc < 2) {
        fprintf(stderr, "requires a port!\n");
        return EXIT_FAILURE;
    }

    if(enet_initialize()) {
        fprintf(stderr, "failed to initialize enet!\n");
        return EXIT_FAILURE;
    }

    addr.host = ENET_HOST_ANY;
    addr.port = atoi(argv[1]);
    printf("listing on *:%d\n", addr.port);
    server = enet_host_create(&addr,
            16, /* allow up to 16 clients */
            2,  /* each client is allowed 2 channels (0 and 1) */
            0,  /* no downstream limit */
            0); /* no upstream limit */
    if(!server) {
        fprintf(stderr, "failed to initialize enet server!\n");
        return EXIT_FAILURE;
    }

    if(init())
        ret = server_loop();
    else {
        fprintf(stderr, "failed to initialize\n");
        ret = 1;
    }

    enet_host_destroy(server);
    enet_deinitialize();
    return ret;
}
