#pragma once

#define DEBUG 0

#include <err.h> /* errx */
#include <stdlib.h> /* calloc, free, realloc */
#include <stdio.h> /* printf */
#include <string.h> /* memmove, memset */

/* a 3d grid containing N^3 Ts
 *
 * fixed_cube internally stores an array of T
 * fixed_cube will NOT call constructor or destructor for the Ts
 * it will however ensure that the memory is zerod (memset)
 *
 */
template <class T, int N>
struct fixed_cube {
    /* contents of grid
     * 3d array of T
     *
     * size determined by xy, yz, zd
     * the dimensions being [x][y][z]
     *
     * to convert co-ords (x,y,z) into a single [index]
     * you do
     * [ x + (y * xd) + (z * xd * yd) ]
     */
    T contents[N][N][N];

    /* return a *T at coordinates (x, y, z)
     * or null on error
     *
     * will check bounds
     */
    T * get(unsigned int x, unsigned int y, unsigned int z);

};

template <class T, int N>
T *
fixed_cube<T, N>::get(unsigned int x, unsigned int y, unsigned int z)
{
    if( x >= N ||
        y >= N ||
        z >= N ){
#if DEBUG
        printf("fixed_cube::get OUT OF RANGE: x: %u/%u, y: %u/%u, z: %u/%u\n", x, N, y, N, z, N);
#endif
        return 0;
    }

    if( ! this->contents )
        errx(1, "fixed_cube::get called, but this->contents is empty");

    return &( contents[x][y][z] );
}

