# engineers-nightmare [![Build Status](https://travis-ci.org/engineers-nightmare/engineers-nightmare.svg)](https://travis-ci.org/engineers-nightmare/engineers-nightmare)

A game about keeping a spaceship from falling apart


# current state

This is the view you are greeted with on game start:

![starting](https://raw.githubusercontent.com/engineers-nightmare/engineers-nightmare/master/misc/en-start-2015-04-23.png)

And here is a mess I made by removing surfaces and blocks:

![mess](https://raw.githubusercontent.com/engineers-nightmare/engineers-nightmare/master/misc/en-mess-2015-04-23.png)


# building and running

build:

    cmake .
    make

test:

    make test

run:

    ./nightmare


# dependencies

* SDL2
* SDL2_image
* libepoxy
* assimp
* glm
* bullet

NB: this above list must be kept in sync with `.travis.yml`


