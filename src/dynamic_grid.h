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

    /* upper bounds of each dimension */
    unsigned int xd, yd, zd;

    /* this will zero out the contents but will NOT call a constructor on T */
    dynamic_grid(unsigned int xdim, unsigned int ydim, unsigned int zdim);

    /* this will free contents but will NOT call a destructor on T */
    ~dynamic_grid(void);

    /* return a *T at coordinates (x, y, z)
     * or null on error
     *
     * will check bounds
     */
    T * get(unsigned int x, unsigned int y, unsigned int z);

    /* axis to extend */
    enum extend_axis {
        extend_x,
        extend_y,
        extend_z
    };

    /* direction to extend
     * high means that the new memory ends up 'after' the existing elements
     * low means that the new memory ends up 'before' the existing elements
     *
     * high = higher number along the axis
     * low = lower number along the axis
     *
     */
    enum extend_direction {
        extend_high,
        extend_low
    };

    /* returns 0 on error
     * returns 1 on success
     *
     * FIXME will currently call errx on failed alloc
     *      decide on correct error handling process
     *
     * extend the existing chunk
     *  along the axis `axis`
     *  in the direction `direction`
     *  by `extend_by` units
     *
     * will call new() on each of the new elements
     *
     */
    int extend(extend_axis axis, extend_direction direction, unsigned int extend_by);

private:

    int _extend_x(extend_direction direction, unsigned int extend_by);
    int _extend_y(extend_direction direction, unsigned int extend_by);
    int _extend_z(extend_direction direction, unsigned int extend_by);
};


template <class T>
dynamic_grid<T>::dynamic_grid(unsigned int xdim, unsigned int ydim, unsigned int zdim)
    : xd(xdim), yd(ydim), zd(zdim), contents(0)
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
dynamic_grid<T>::~dynamic_grid()
{
    /* clean up allocated region */
    free(this->contents);
}

template <class T>
T *
dynamic_grid<T>::get(unsigned int x, unsigned int y, unsigned int z)
{
    if( x >= this->xd ||
        y >= this->yd ||
        z >= this->zd ){
#if DEBUG
        printf("dynamic_grid::get OUT OF RANGE: x: %u/%u, y: %u/%u, z: %u/%u\n", x, this->xd, y, this->yd, z, this->zd);
#endif
        return 0;
    }

    if( ! this->contents )
        errx(1, "dynamic_grid::get called, but this->contents is empty");

    return &( contents[ x + (y * this->xd) + (z * this->xd * this->yd) ] );
}

template <class T>
int
dynamic_grid<T>::extend(extend_axis axis, extend_direction direction, unsigned int extend_by)
{
    if( ! this->contents )
        errx(1, "dynamic_grid::extend called, but this->contents is empty");

    switch( axis ){
        case extend_x:
            return this->_extend_x(direction, extend_by);
            break;

        case extend_y:
            return this->_extend_y(direction, extend_by);
            break;

        case extend_z:
            return this->_extend_z(direction, extend_by);
            break;

        default:
            errx(1, "dynamic_grid::extend : impossible extend_axis supplied");
            break;
    }
}

template <class T>
int
dynamic_grid<T>::_extend_x(extend_direction direction, unsigned int extend_by)
{
    errx(1, "dynamic_grid::_extend_x : not implemented yet");
    return 0;
}

template <class T>
int
dynamic_grid<T>::_extend_y(extend_direction direction, unsigned int extend_by)
{
    errx(1, "dynamic_grid::_extend_y : not implemented yet");
    return 0;
}

template <class T>
int
dynamic_grid<T>::_extend_z(extend_direction direction, unsigned int extend_by)
{
    /* extending along Z is the simplest case as each Z is
     * a contiguous block with dimension (this->xd * this->yd)
     *
     * that is to say every element within z=0 can be found via
     *
     *  for( int x=0; x < this->xd; ++x ){
     *      for( int y=0; y < this->yd; ++y ){
     *          this->contents[ x + (y * xdim) ];
     *      }
     *  }
     *
     * and every element at z=4 can be found via
     *
     *  #DEFINE Z 4
     *  for( int x=0; x < this->xd; ++x ){
     *      for( int y=0; y < this->yd; ++y ){
     *          this->contents[ x + (y * xdim) + (Z * xdim * ydim)  ];
     *      }
     *  }
     *
     * so extending z is simply a matter of adding extend_by quantities
     * of (xdim * ydim) at either the end (extend_high) or the front (extend_low)
     * of this->contents
     *
     */

    /* pointer to new region
     * we eventually write back to this->contents */
    T * new_contents = 0;

    /* pointer to just the new sub-area of new_contents */
    T * new_region = 0;

    /* various size calculations */
    unsigned int old_size = this->xd * this->yd * this->zd;
    unsigned int new_zd = this->zd + extend_by;
    unsigned int new_size = this->xd * this->yd * new_zd;
    unsigned int growth = new_size - old_size;

    /* counter used for placement new */
    unsigned int i = 0;

    /* FIXME gimping temporary for refactoring */
    errx(1, "dynamic_grid::_extend_z : refactoring, do not use");

    if( extend_by <= 0 ){
        /* waste of time */
        return 0;
    }

    new_contents = (T*) realloc(this->contents, new_size * sizeof(T));
    if( ! new_contents ){
        /* FIXME decide on error handling */
        errx(1, "dynamic_grid::_extend_z : realloc failed");
        return 1;
    }

    switch( direction ) {
        case extend_high:
            /* no shuffling required */

            /* point to first element of freshly allocated region
             * last `growth` elements of new_contents
             */
            new_region = &( new_contents[old_size] );

            break;

        case extend_low:
            /* shuffling required
             * shunt contents down by growth (extend_by * this->yd * this->xd)
             *
             * FIXME how does memmove communicate failure
             */
            memmove(&(new_contents[growth]), new_contents, old_size * sizeof(T));

            /* point to first element of now blank region
             * first `growth` elements of new_contents
             */
            new_region = new_contents;

            break;

        default:
            errx(1, "dynamic_grid::_extend_x : impossible extend_direction supplied");
            break;

    }

    /* memset new region */
    memset(new_region, 0, growth * sizeof(T));

    this->contents = new_contents;
    this->zd = new_zd;

    /* success */
    return 0;
}

