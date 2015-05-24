# Engineers-nightmare [![Build Status](https://travis-ci.org/engineers-nightmare/engineers-nightmare.svg)](https://travis-ci.org/engineers-nightmare/engineers-nightmare)

A game about keeping a spaceship from falling apart


# Current state

So far, have implemented solid player movement, including jumping and crouching, crawling inside blocks, etc; tools for
placing and removing block scaffolding, surfaces of several types, block-mounted and surface-mounted entities of several
types; Lightfield propagated from certain entities.

![example](https://raw.githubusercontent.com/engineers-nightmare/engineers-nightmare/master/misc/en-2015-05-18-1.png)


# Building and running

## Dependencies

 * assimp
 * bullet
 * freetype6
 * glm
 * libconfig
 * libepoxy
 * mman-win32 (For Windows build)
 * SDL2
 * SDL2_image

NB: this above list must be kept in sync with `.travis.yml`

## Building on Linux

build:

    cmake .
    make

test:

    make test

run:

    ./nightmare

## Building on Windows

The following instructions were written to get a working build under `Visual Studio 2013`. If you can do better, or if something is missing, please submit a PR with the fix.

Create some directory `en` for Engineers Nightmare

`git clone` this project into `en`

### Dependency Setup

Create directories `assimp`, `bullet`, `freetype6`, `glm`, `libconfig`, `libepoxy`, `mman-win32`, `sdl2`, `sdl2_image`
`mkdir assimp bullet freetype6 glm libconfig libepoxy mman-win32 sdl2 sdl2_image`

* Into `assimp` extract from `assimp-N_no_test_models.zip` (N == 3.1.1 as of May 2015) from http://sourceforge.net/projects/assimp/files
  * Run `cmake -DCMAKE_C_FLAGS_MINSIZEREL:STRING="/MT /O1 /Ob1 /D NDEBUG" -DASSIMP_BUILD_ASSIMP_TOOLS:BOOL="0" -DBUILD_SHARED_LIBS:BOOL="0" -DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING="/MT /Zi /O2 /Ob1 /D NDEBUG" -DCMAKE_CXX_FLAGS_DEBUG:STRING="/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1" -DCMAKE_C_FLAGS_RELEASE:STRING="/MT /O2 /Ob2 /D NDEBUG" -DCMAKE_CXX_FLAGS_MINSIZEREL:STRING="/MT /O1 /Ob1 /D NDEBUG" -DCMAKE_C_FLAGS_DEBUG:STRING="/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1" -DASSIMP_BUILD_STATIC_LIB:BOOL="1" -DASSIMP_BUILD_TESTS:BOOL="0" -DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING="/MT /Zi /O2 /Ob1 /D NDEBUG" -DCMAKE_CXX_FLAGS_RELEASE:STRING="/MT /O2 /Ob2 /D NDEBUG"  .` in `assimp` directory
  * Run `start Assimp.sln'
  * Build `Release` and `Debug` modes on `ALL_BUILD` project.
* Into `bullet` extract from `Latest Release` at https://github.com/bulletphysics/bullet3/releases
  * Run `cmake -DBUILD_SHARED_LIBS:BOOL="0" -DBUILD_UNIT_TESTS:BOOL="0" -DBUILD_BULLET2_DEMOS:BOOL="0" -DBUILD_EXTRAS:BOOL="0" -DUSE_MSVC_RUNTIME_LIBRARY_DLL:BOOL="1" -DBUILD_CPU_DEMOS:BOOL="0" -DBUILD_OPENGL3_DEMOS:BOOL="0" .` in `bullet` directory
  * Run `start BULLET_PHYSICS.sln`
  * Build `Release` and `Debug` mode on `ALL_BUILD` project.
* Into `freetype6` extract (at a minimum) `bin`, `lib`, `include` from `Binaries` release at http://gnuwin32.sourceforge.net/packages/freetype.htm
* Into `glm` extract from `Latest Release` at https://github.com/g-truc/glm/releases
* Into `libconfig` extract from `1.4.9` source at `http://freecode.com/projects/libconfigduo/releases`
  * Run `start libconfig.sln`
  * Build `Release` and `Debug` modes on `libconfig` project
* Into `libepoxy` extract `include`, `win32`, `win64` from `*-win32.zip` of `Latest Release` at https://github.com/anholt/libepoxy/releases
* Into `mman-win32` extract from https://github.com/RobotCaleb/mman-win32/archive/master.zip
  * Open `mman.vcproj` and build `Release` and `Debug` modes on `mman` project
* Into `sdl2` extract from `Development Libraries -> Windows -> *-VC.zip` at https://www.libsdl.org/download-2.0.php
* Into `sdl2_image` extract from `Development Libraries -> Windows -> *-VC.zip` at https://www.libsdl.org/projects/SDL_image/
