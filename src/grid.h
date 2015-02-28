#pragma once

/* a 3d grid containing N^3 Ts
 */
template <class T, unsigned int N>
struct grid_3d {
    T contents[N][N][N];

    T * get(unsigned int x, unsigned int y, unsigned int z);
};


template <class T, unsigned int N>
T *
grid_3d<T, N>::get(unsigned int x, unsigned int y, unsigned int z)
{
    if( x >= N ||
        y >= N ||
        z >= N )
        return 0;

    return &( contents[x][y][z] );
}
