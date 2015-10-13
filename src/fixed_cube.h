#pragma once

#define DEBUG 0

#ifndef _WIN32
#include <err.h> /* errx */
#else
#include "winerr.h"
#endif

#include <assert.h>

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

    fixed_cube()
    {
        memset(contents, 0, sizeof(contents));
    }
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
    T * get(unsigned int x, unsigned int y, unsigned int z)
    {
        if( x >= N ||
            y >= N ||
            z >= N ){
            assert(!"out of range");
            return nullptr;
        }

        return &( contents[x][y][z] );
    }

};
