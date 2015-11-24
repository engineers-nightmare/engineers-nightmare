#include <string>

struct packet_reader {

    uint8_t *buf;
    size_t len;
    size_t ofs;

    packet_reader(uint8_t *buf, size_t len)
        : buf(buf), len(len), ofs(0) {
    }

    /* for all types that don't require special handling, write directly.
    it's a little endian world. where the wire representation is not
    bit-exact, expected to provide an overload.
    */
    template<typename T>
    packet_reader & get(T& value) {
        assert(ofs + sizeof(T) <= len);
        memcpy(&value, buf + ofs, sizeof(T));
        ofs += sizeof(T);

        return *this;
    }

    packet_reader & get(std::string &str);
};
