#pragma once

#define DEBUG 0

#include <err.h> /* errx */
#include <stdlib.h> /* calloc, free, realloc */
#include <stdio.h> /* printf */
#include <string.h> /* memmove, memset */

/* a 3d grid containing N^3 Ts
 *
 * NOTE that grid_3d internally stores a grid of pointers to T
 * and the grid_3d constructor will automatically also allocate the Ts
 * the grid_3d destructor will destroy all these allocated Ts
 */
template <class T>
struct grid_3d {
    /* contents of grid
     * 3d array of pointers to T
     *
     * size determined by xy, yz, zd
     * the dimensions being [x][y][z]
     *
     * to convert co-ords (x,y,z) into a single [index]
     * you do
     * [ x + (y * xd) + (z * xd * yd) ]
     */
    T ** contents;

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

    this->contents = (T**) calloc(sizeof(T*), xdim * ydim * zdim);

    if( ! this->contents )
        errx(1, "grid_3d::grid_3d calloc failed");

    /* allocate and initialise contents */
    for(i=0; i< (this->xd * this->yd * this->zd); ++i){
        this->contents[i] = new T();
    }
}

template <class T>
grid_3d<T>::~grid_3d()
{
    int i=0;

    /* clean up each sub object */
    for(i=0; i< (this->xd * this->yd * this->zd); ++i){
        delete this->contents[i];
    }

    /* clean up allocated region */
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

    if( ! this->contents )
        errx(1, "grid_3d::get called, but this->contents is empty");

    return contents[ x + (y * this->xd) + (z * this->xd * this->yd) ];
}

template <class T>
int
grid_3d<T>::extend(extend_axis axis, extend_direction direction, unsigned int extend_by)
{
    if( ! this->contents )
        errx(1, "grid_3d::extend called, but this->contents is empty");

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
    /* pointer to new region
     * we eventually write back to this->contents */
    T ** new_contents = 0;

    /* pointer to just the new sub-area of new_contents */
    T ** new_region = 0;

    /* various size calculations */
    unsigned int old_size = this->xd * this->yd * this->zd;
    unsigned int new_zd = this->zd + extend_by;
    unsigned int new_size = this->xd * this->yd * new_zd;
    unsigned int growth = new_size - old_size;

    /* counter used for placement new */
    unsigned int i = 0;


    if( extend_by <= 0 ){
        /* waste of time */
        return 0;
    }

    new_contents = (T**) realloc(this->contents, new_size * sizeof(T**));
    if( ! new_contents ){
        /* FIXME decide on error handling */
        errx(1, "grid_3d::_extend_z : realloc failed");
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
            memmove(&(new_contents[growth]), new_contents, old_size * sizeof(T**));

            /* point to first element of now blank region
             * first `growth` elements of new_contents
             */
            new_region = new_contents;

            break;

        default:
            errx(1, "grid_3d::_extend_x : impossible extend_direction supplied");
            break;

    }

    /* memset new region */
    memset(new_region, 0, growth * sizeof(T*));

    /* allocate and initialise elements in new region
     * we know there are `growth` items in there
     */
    for( i=0; i < growth; ++i ){
        new_region[i] = new T();
    }

    this->contents = new_contents;
    this->zd = new_zd;

    /* success */
    return 0;
}

