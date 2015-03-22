#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <err.h>

#include "common.h"
#include "blob.h"


blob::blob(char const *filename)
    : fd(-1), data(0), len(0)
{
    fd = open(filename, 0);
    if (fd == -1)
        err(1, "Failed to open blob: %s\n", filename);

    struct stat st;
    fstat(fd, &st);

    len = st.st_size;
    /* client should not expect the memory to be writable, but we'll set this up as CoW
     * in case they mprotect() it. */
    data = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);

    if (data == MAP_FAILED)
        err(1, "Failed to mmap contents of blob: %s\n", filename);
}

blob::~blob()
{
    munmap(data, len);
    close(fd);
}
