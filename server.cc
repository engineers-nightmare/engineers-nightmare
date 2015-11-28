#include <stdio.h>
#include <stdlib.h>

#include "src/network.h"
#include "src/ship_space.h"
#include "src/packet_reader.h"

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
ship_space *ship;

/* todo: this is a hack. fix */
hw_mesh *door_hw;

struct peer_info {
    unsigned vsn_str;
};

enum msg_ret_type {
    MSG_OK,
    MSG_PEER_DISCONNECT,
    MSG_UNKNOWN,
};

static ENetHost *server;

int
init(void)
{
    ship = ship_space::mock_ship_space();
    if(!ship) {
        fprintf(stderr, "failed to load mock ship space\n");
        return 0;
    }

    ship->rebuild_topology();

    return 1;
}

/* handle the submessage of a SERVER_MSG */
void
handle_server_message(ENetEvent *event, packet_reader &pr,
    message_subtype_server subtype)
{
    struct peer_info *pi;
    int i;

    switch(subtype) {
        case message_subtype_server::client_version:
        {
            uint8_t major;
            uint8_t minor;
            uint8_t patch;

            pr.get(major).get(minor).get(patch);

            printf("client version %d.%d.%d ", major, minor, patch);

            if (CHECK_VERSION(major, minor, patch)) {
                printf("compatible\n");
                send_server_version(event->peer, VSN_MAJOR, VSN_MINOR,
                    VSN_PATCH);

                pi = new struct peer_info;
                pi->vsn_str = MAKE_VERSION_CHECK(major, minor, patch);
                event->peer->data = pi;
            }
            else {
                printf("incompatible!\n");
                send_incompatible_version(event->peer, MIN_CLIENT_MAJOR,
                    MIN_CLIENT_MINOR, MIN_CLIENT_PATCH);
            }
            break;
        }
        case message_subtype_server::slot_request:
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
        default:
            printf("unknown message(0x%02X)\n", subtype);
    }
}

/* handle the submessage of a SHIP_MSG */
void
handle_ship_message(ENetEvent *event, packet_reader &pr, message_subtype_ship subtype)
{
    switch(subtype) {
        case message_subtype_ship::all_ship_request:
            printf("ship space requested!\n");
            if(!event->peer->data) {
                send_not_in_slot(event->peer);
                return;
            }

            // TODO: send more than one chunk
            for (auto c : ship->chunks) {

                auto vbuf = ship->serialize_chunk(c.first);

                send_ship_chunk(event->peer, c.first, vbuf);

                /* alloced in serialize_chunk() */
                delete vbuf;
            }
            reply_whole_ship(event->peer);
            break;
        default:
            printf("unknown messages(0x%02X)\n", subtype);
    }
}

/* handle the submessage of a UPDATE_MSG */
void
handle_update_message(ENetEvent *event, packet_reader &pr,
    message_subtype_update subtype)
{
    glm::ivec3 vec;
    glm::ivec3 pvec;

    switch(subtype) {
        case message_subtype_update::set_block_type:
        {
            printf("set block type!\n");
            
            block_type type;

            pr.get(vec);
            pr.get(type);

            printf("setting block at %d,%d,%d to %d\n", vec.x, vec.y, vec.z, type);

            /* todo: verify block can be set */
            ship->set_block(pvec, type);

            for (int i = 0; i < MAX_SLOTS; i++) {
                if (peers[i]) {
                    send_data(peers[i], event->packet->data, event->packet->dataLength);
                }
            }

            break;
        }
        case message_subtype_update::set_surface_type:
        {
            printf("set texture type!\n");

            surface_index index;
            surface_type type;

            pr.get(vec);
            pr.get(pvec);
            pr.get(index);
            pr.get(type);

            printf("setting surface at %d,%d,%d|%d,%d,%d to %d on %d\n",
                vec.x, vec.y, vec.z, pvec.x, pvec.y, pvec.z, type, index);

            ship->set_surface(vec, pvec, index, type);

            for (int i = 0; i < MAX_SLOTS; i++) {
                if (peers[i])
                    send_data(peers[i], event->packet->data, 28);
            }
            break;
        }
        default:
            printf("unknown message(0x%02X)\n", subtype);
    }
}

/*  handle any incoming messages */
void
handle_message(ENetEvent *event)
{
    printf("[%x:%u] ", event->peer->address.host, event->peer->address.port);

    packet_reader pr(event->packet->data, event->packet->dataLength);

    message_type type;
    pr.get(type);

    switch(type) {
        case message_type::server:
        {
            message_subtype_server subtype;
            pr.get(subtype);
            printf("server message(0x%02x): ", subtype);
            handle_server_message(event, pr, subtype);
            break;
        }
        case message_type::ship:
        {
            message_subtype_ship subtype;
            pr.get(subtype);
            printf("ship message(0x%02x): ", subtype);
            handle_ship_message(event, pr, subtype);
            break;
        }
        case message_type::update:
        {
            message_subtype_update subtype;
            pr.get(subtype);
            printf("update message(0x%02x): ", subtype);
            handle_update_message(event, pr, subtype);
            break;
        }
        default:
            printf("unknown message(0x%02x)\n", type);
    }
}

/* handle a remote disconnect. remove all references to the cliet */
void
handle_disconnect(ENetEvent *event)
{
    int i;

    printf("[%x:%u] disconnected\n", event->peer->address.host,
            event->peer->address.port);
    if(event->peer->data){
        for(i = 0; i < MAX_SLOTS; i++) {
            if(peers[i] && peers[i]->connectID == event->peer->connectID) {
                peers[i] = nullptr;
                break;
            }
        }
        delete (struct peer_info*)event->peer->data;
    }
}

/* listen/dispatch loop */
int
server_loop()
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
                    event.peer->data = nullptr;
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
    } while(server_ret >= 0);

    if(server_ret < 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
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
    printf("listening on *:%d\n", addr.port);
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
