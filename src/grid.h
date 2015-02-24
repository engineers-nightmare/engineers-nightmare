#pragma once

/* a 3d grid containing N^3 Ts
 */
template <class T, unsigned int N>
struct grid_3d {
    T contents[N][N][N];
};


