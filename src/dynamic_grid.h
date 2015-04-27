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

    /* check if the given co-ords are contained within this DG
     * this is needed as a DG may be sparse
     *
     * returns true if co-ords are contained within
     * returns false otherwise
     */
    bool within(int ex, int ey, int ez);

    /* resize the grid to new dimensions
     *
     * this will move everything around to ensure that
     * items will still be found at their old x, y, z
     */
    void resize(int nxd, int nyd, int nzd);
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

/* resize the grid to new dimensions
 *
 * this will move everything around to ensure that
 * items will still be found at their old x, y, z
 */
template <class T>
void
dynamic_grid<T>::resize(int nxd, int nyd, int nzd)
{
    /* size of each dimension */
    unsigned int x = 0,
                 y = 0,
                 z = 0;

    T * new_contents = 0;

    /* check if we were asked to shrink */
    if( nxd < this->xd ||
        nyd < this->yd ||
        nzd < this->zd ){

        /* shrinking is unsupported */
        printf("dynamic_grid::resize: current (%d, %d, %d) asked to shrink to (%d, %d, %d)\n",
                this->xd, this->yd, this->zd, nxd, nyd, nzd);
        errx(1, "dynamic_grid::resize does not support shrinking");
    }

    /* check if we were asked to do nothing */
    if( nxd == this->xd &&
        nyd == this->yd &&
        nzd == this->zd ){

        /* nothing to do */
        return;
    }

    new_contents = (T*) calloc(sizeof(T), nxd * nyd * nzd);

    if( ! new_contents ){
        errx(1, "dynamic_grid::resize calloc failed");
    }

    /* copy everything over */
    for( z=0; z<zd; ++z ){
        for( y=0; y<yd; ++y ){
            for( x=0; x<xd; ++x ){
                new_contents[ x + (y * nxd) + (z * nxd * nyd) ] = this->contents[ x + (y * this->xd) + (z * this->xd * this->yd) ];

            }
        }
    }

    /* nuke our old contents */
    free(this->contents);

    /* update all our instance variables */
    this->contents = new_contents;
    this->xd = nxd;
    this->yd = nyd;
    this->zd = nzd;
}

/* check if the given co-ords are contained within this DG
 * this is needed as a DG may be sparse
 *
 * returns true if co-ords are contained within
 * returns false otherwise
 */
template <class T>
bool
dynamic_grid<T>::within(int ex, int ey, int ez)
{
    /* translate to internal co-ords*/
    int ix, iy, iz;
    ix = ex - this->xo;
    iy = ey - this->yo;
    iz = ez - this->zo;

    /* check if not within space */
    if( ix >= this->xd ||
        iy >= this->yd ||
        iz >= this->zd ||
        ix < 0         ||
        iy < 0         ||
        iz < 0         ){
        return false;
    }

    /* otherwise it is contained within */
    return true;
}


