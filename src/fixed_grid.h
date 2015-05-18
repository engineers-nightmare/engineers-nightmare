#pragma once

#define DEBUG 0

#ifndef _WIN32
#include <err.h> /* errx */
#else
#include "winerr.h"
#endif

#include <stdlib.h> /* calloc, free, realloc */
#include <stdio.h> /* printf */
#include <string.h> /* memmove, memset */

/* a 3d grid containing N^3 Ts
 *
 * fixed_grid internally stores an array of T
 * fixed_grid will NOT call constructor or destructor for the Ts
 * it will however ensure that the memory is zerod (memset)
 *
 */
template <class T>
struct fixed_grid {
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
    T * contents;

    /* upper bounds of each dimension */
    unsigned int xd, yd, zd;

    /* this will zero out the contents but will NOT call a constructor on T */
    fixed_grid(unsigned int xdim, unsigned int ydim, unsigned int zdim);

    /* this will free contents but will NOT call a destructor on T */
    ~fixed_grid(void);

    /* return a *T at coordinates (x, y, z)
     * or null on error
     *
     * will check bounds
     */
    T * get(unsigned int x, unsigned int y, unsigned int z);

};


template <class T>
fixed_grid<T>::fixed_grid(unsigned int xdim, unsigned int ydim, unsigned int zdim)
    : xd(xdim), yd(ydim), zd(zdim), contents(0)
{
    int i=0;
    void *place;

#if DEBUG
    printf("fixed_grid::fixed_grid given xdim %u, ydim %u, zdim %u\n", xdim, ydim, zdim);
    printf("fixed_grid::fixed_grid given xd %u, yd %u, zd %u\n", this->xd, this->yd, this->zd);
#endif

    this->contents = (T*) calloc(sizeof(T), xdim * ydim * zdim);

    if( ! this->contents )
        errx(1, "fixed_grid::fixed_grid calloc failed");
}

template <class T>
fixed_grid<T>::~fixed_grid()
{
    /* clean up allocated region */
    free(this->contents);
}

template <class T>
T *
fixed_grid<T>::get(unsigned int x, unsigned int y, unsigned int z)
{
    if( x >= this->xd ||
        y >= this->yd ||
        z >= this->zd ){
#if DEBUG
        printf("fixed_grid::get OUT OF RANGE: x: %u/%u, y: %u/%u, z: %u/%u\n", x, this->xd, y, this->yd, z, this->zd);
#endif
        return 0;
    }

    if( ! this->contents )
        errx(1, "fixed_grid::get called, but this->contents is empty");

    return &( contents[ x + (y * this->xd) + (z * this->xd * this->yd) ] );
}

