# Engineers-nightmare [![Build Status](https://travis-ci.org/engineers-nightmare/engineers-nightmare.svg)](https://travis-ci.org/engineers-nightmare/engineers-nightmare)

A game about keeping a spaceship from falling apart


# Current state

So far, have implemented solid player movement, including jumping and crouching, crawling inside blocks, etc; tools for
placing and removing block scaffolding, surfaces of several types, block-mounted and surface-mounted entities of several
types; Lightfield propagated from certain entities.

![example](https://raw.githubusercontent.com/engineers-nightmare/engineers-nightmare/master/misc/en-2015-05-18-1.png)


# Building and running

build:

    cmake .
    make

test:

    make test

run:

    ./nightmare


# Dependencies

 * assimp
 * bullet
 * freetype6
 * glm
 * libepoxy
 * SDL2
 * SDL2_image

NB: this above list must be kept in sync with `.travis.yml`


