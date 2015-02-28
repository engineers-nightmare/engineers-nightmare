#pragma once

#define DEBUG 0

#include <err.h> /* errx */
#include <stdlib.h> /* calloc, free */
#include <stdio.h> /* puts, printf */

/* placement new
 * http://stackoverflow.com/questions/3156778/no-matching-function-for-call-to-operator-new
 * http://isocpp.org/wiki/faq/dtors#placement-new
 */
#include <new>

/* a 3d grid containing N^3 Ts
 */
template <class T>
struct grid_3d {
    /* contents of grid
     * 3d array
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

    grid_3d(unsigned int xdim, unsigned int ydim, unsigned int zdim);

    ~grid_3d(void);

    /* return a *T at coordinates (x, y, z)
     * or null on error
     *
     * will check bounds
     */
    T * get(unsigned int x, unsigned int y, unsigned int z);
};


template <class T>
grid_3d<T>::grid_3d(unsigned int xdim, unsigned int ydim, unsigned int zdim)
    : xd(xdim), yd(ydim), zd(zdim), contents(0)
{
    int i=0;
    void *place;

#if DEBUG
    printf("grid_3d::grid_3d given xdim %u, ydim %u, zdim %u\n", xdim, ydim, zdim);
    printf("grid_3d::grid_3d given xd %u, yd %u, zd %u\n", this->xd, this->yd, this->zd);
#endif

    this->contents = (T*) calloc(sizeof(T), xdim * ydim * zdim);

    if( ! this->contents )
        errx(1, "grid_3d::grid_3d calloc failed");

    /* initialise contents */
    for(i=0; i< (this->xd * this->yd * this->zd); ++i){
        /* http://isocpp.org/wiki/faq/dtors#placement-new */
        /* FIXME assuming we do not need to check return value
         */
        new( &(this->contents[i]) ) T();
    }
}

template <class T>
grid_3d<T>::~grid_3d()
{
    int i=0;
    /* must call destructor manually on each object */
    for(i=0; i< (this->xd * this->yd * this->zd); ++i){
        /* http://isocpp.org/wiki/faq/dtors#placement-new */
        this->contents[i].~T();
    }

    free(this->contents);
}

template <class T>
T *
grid_3d<T>::get(unsigned int x, unsigned int y, unsigned int z)
{
    if( x >= this->xd ||
        y >= this->yd ||
        z >= this->zd ){
#if DEBUG
        printf("grid_3d::get OUT OF RANGE: x: %u/%u, y: %u/%u, z: %u/%u\n", x, this->xd, y, this->yd, z, this->zd);
#endif
        return 0;
    }

    if( ! contents ){
#if DEBUG
        puts("grid_3d::get NO CONTENTS");
#endif
        return 0;
    }

    return &( contents[ x + (y * this->xd) + (z * this->xd * this->yd) ] );
}
