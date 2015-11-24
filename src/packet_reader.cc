#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <glm/glm.hpp>

#include "packet_reader.h"

void
packet_reader::get(std::string &str) {
    /* abomination though it is: an example, a length-prefixed string */
    /* simple, but not particularly good. */
    uint32_t size = (uint32_t)str.size();
    assert(ofs + size + sizeof(size) <= len);
    get(size);
    memcpy((void*)str.data(), buf + ofs, size);
    ofs += size;
}
