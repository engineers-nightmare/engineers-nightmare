#pragma once

struct blob {
    int fd;
    void *data;
    size_t len;

    blob(char const *filename);
    ~blob();
};
