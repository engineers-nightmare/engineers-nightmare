#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <glm/glm.hpp>
#include "packet_writer.h"

packet_writer &
packet_writer::put(std::string const &str) {
    uint32_t size = (uint32_t)str.size();
    assert(ofs + size + sizeof(size) <= len);
    put(size);
    memcpy(buf + ofs, str.data(), size);
    ofs += size;

    return *this;
}
