#pragma once

#include <stdio.h>

#include "timer.h"

struct scopetimer {
    Timer timer;
    char *name;
    
    scopetimer(char *name) {
        this->name = name;
    }

    ~scopetimer() {
        auto dt = timer.peek();
        printf("%s : %f\n", name, dt.delta);
    }
};
