#include <string>
#include <glm/glm.hpp>
#include <vector>


struct packet_writer {
    uint8_t *buf;
    size_t len;
    size_t ofs;

    packet_writer()
        : buf(nullptr), len(0), ofs(0) {
    }

    packet_writer(uint8_t *buf, size_t len)
        : buf(buf), len(len), ofs(0) {
    }

    /* for all types that don't require special handling, write directly.
    it's a little endian world. where the wire representation is not
    bit-exact, expected to provide an overload.
    */
    template<typename T>
    void put(T const &t) {
        /* caller promised the buffer was large enough */
        assert(ofs + sizeof(T) <= len);
        memcpy(buf + ofs, &t, sizeof(T));
        ofs += sizeof(T);
    }

    /* abomination though it is: an example, a length-prefixed string */
    /* simple, but not particularly good. */
    void put(std::string const &t);
};
