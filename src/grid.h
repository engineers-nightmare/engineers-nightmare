#pragma once

#define DEBUG 0

#include <err.h> /* errx */
#include <stdlib.h> /* calloc, free, realloc */
#include <stdio.h> /* puts, printf */
#include <string.h> /* memmove */

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

template <class T>
int
grid_3d<T>::extend(extend_axis axis, extend_direction direction, unsigned int extend_by)
{
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
            errx(1, "grid_3d::extend : impossible extend_axis supplied");
            break;
    }
}

template <class T>
int
grid_3d<T>::_extend_x(extend_direction direction, unsigned int extend_by)
{
    errx(1, "grid_3d::_extend_x : not implemented yet");
    return 0;
}

template <class T>
int
grid_3d<T>::_extend_y(extend_direction direction, unsigned int extend_by)
{
    errx(1, "grid_3d::_extend_y : not implemented yet");
    return 0;
}

template <class T>
int
grid_3d<T>::_extend_z(extend_direction direction, unsigned int extend_by)
{
    T * new_contents = 0;
    unsigned int new_zd = this->zd + extend_by;
    unsigned int new_size = this->xd * this->yd * new_zd;

    new_contents = (T*) realloc(this->contents, new_size);
    if( ! new_contents ){
        /* FIXME decide on error handling */
        errx(1, "grid_3d::_extend_z : realloc failed");
        return 1;
    }

    switch( direction ) {
        case extend_high:
            /* no shuffling required */
            break;

        case extend_low:
            /* shuffling required */
            /* TODO */
            errx(1, "grid_3d::_extend_x : extend_low not implemented yet");
            break;

        default:
            errx(1, "grid_3d::_extend_x : impossible extend_direction supplied");
            break;

    }

    this->contents = new_contents;
    this->zd = new_zd;
    /* success */
    return 0;
}

