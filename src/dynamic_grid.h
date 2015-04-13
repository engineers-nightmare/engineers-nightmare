#pragma once

#define DEBUG 0

#include <err.h> /* errx */
#include <stdlib.h> /* calloc, free, realloc */
#include <stdio.h> /* printf */
#include <string.h> /* memmove, memset */

/* a 3d grid containing N^3 Ts
 *
 * dynamic_grid can be extended at runtime
 *
 * dynamic_grid internally stores an array of T
 * dynamic_grid will NOT call constructor or destructor for the Ts
 * it will however ensure that the memory is zerod (memset)
 *
 * dynamic grid externally exposes and allows for negative indexes
 * internally however it is an array, so we store 'offsets' for each axis
 * to convert between Internal {x y z} and External {x y z}
 *
 * functions taking xyz will distinguish between them:
 *  external: ex, ey, ez
 *  internal: ix, iy, iz
 *
 */
template <class T>
struct dynamic_grid {
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

    /* size of each dimension */
    unsigned int xd, yd, zd;

    /* offsets within each dimension */
    unsigned int xo, yo, zo;

    /* this will zero out the contents but will NOT call a constructor on T
     * this creates a dynamic grid from 0,0,0 to xdim,ydim,zdim
     */
    dynamic_grid(unsigned int xdim, unsigned int ydim, unsigned int zdim);

    /* this will zero out the contents but will NOT call a constructor on T
     * this creates a dynamic grid from xfrom,yfrom,zfrom to xto,yto,zto
     */
    dynamic_grid(int xfrom, int xto, int yfrom, int yto, int zfrom, int zto);

    /* this will free contents but will NOT call a destructor on T */
    ~dynamic_grid(void);

    /* return a *T at external coordinates (x, y, z)
     * or null on error
     *
     * will check bounds
     */
    T * get(int ex, int ey, int ez);

};


template <class T>
dynamic_grid<T>::dynamic_grid(unsigned int xdim, unsigned int ydim, unsigned int zdim)
    : xd(xdim), yd(ydim), zd(zdim), contents(0), xo(0), yo(0), zo(0)
{
    int i=0;
    void *place;

#if DEBUG
    printf("dynamic_grid::dynamic_grid given xdim %u, ydim %u, zdim %u\n", xdim, ydim, zdim);
    printf("dynamic_grid::dynamic_grid given xd %u, yd %u, zd %u\n", this->xd, this->yd, this->zd);
#endif

    this->contents = (T*) calloc(sizeof(T), xdim * ydim * zdim);

    if( ! this->contents )
        errx(1, "dynamic_grid::dynamic_grid calloc failed");
}

template <class T>
dynamic_grid<T>::dynamic_grid(int xfrom, int xto, int yfrom, int yto, int zfrom, int zto)
    : contents(0)
{
    int i=0;
    void *place;

    if( xfrom >= xto ||
        yfrom >= yto ||
        zfrom >= zto ){
        errx(1, "illegal dimensions supplied");
    }

    this->xd = xto - xfrom;
    this->yd = yto - yfrom;
    this->zd = zto - zfrom;

    this->xo = xfrom;
    this->yo = yfrom;
    this->zo = zfrom;

#if DEBUG
    printf("dynamic_grid::dynamic_grid given xd %u, yd %u, zd %u\n", xd, yd, zd);
    printf("dynamic_grid::dynamic_grid given xd %u, yd %u, zd %u\n", this->xd, this->yd, this->zd);
#endif

    this->contents = (T*) calloc(sizeof(T), this->xd * this->yd * this->zd);

    if( ! this->contents )
        errx(1, "dynamic_grid::dynamic_grid calloc failed");
}



template <class T>
dynamic_grid<T>::~dynamic_grid()
{
    /* clean up allocated region */
    free(this->contents);
}

template <class T>
T *
dynamic_grid<T>::get(int ex, int ey, int ez)
{
    /* translate to internal co-ords*/
    int ix, iy, iz;
    ix = ex - this->xo;
    iy = ey - this->yo;
    iz = ez - this->zo;

    if( ix >= this->xd ||
        iy >= this->yd ||
        iz >= this->zd ||
        ix < 0         ||
        iy < 0         ||
        iz < 0         ){

#if DEBUG
        printf("dynamic_grid::get OUT OF RANGE: x: %u/%u, y: %u/%u, z: %u/%u\n", ix, this->xd, iy, this->yd, iz, this->zd);
#endif
        return 0;
    }

    if( ! this->contents )
        errx(1, "dynamic_grid::get called, but this->contents is empty");

    return &( contents[ ix + (iy * this->xd) + (iz * this->xd * this->yd) ] );
}


