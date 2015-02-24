cmake_minimum_required(VERSION 2.6)
project(nightmare)
add_executable(nightmare src/main.cc)

include(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(EPOXY REQUIRED epoxy)
include_directories(${SDL2_INCLUDE_DIRS} ${EPOXY_INCLUDE_DIRS})
target_link_libraries(nightmare ${SDL2_LIBRARIES} ${EPOXY_LIBRARIES})
