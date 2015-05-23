cmake_minimum_required(VERSION 2.6)
project(nightmare)

# from http://www.cmake.org/Wiki/CMake/Tutorials/C%2B%2B11Flags
if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -DGLM_FORCE_RADIANS")
endif()

add_executable(nightmare main.cc)

# here we compile all of src/*.cc into one library
# this is used below to link both to our main executable
# as well as to link to each of our tests
file(GLOB SOURCE src/*.cc)
add_library(NIGHTMARE ${SOURCE})

include(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(EPOXY REQUIRED epoxy)
PKG_SEARCH_MODULE(ASSIMP REQUIRED assimp)
PKG_SEARCH_MODULE(SDL2_IMAGE REQUIRED SDL2_image)
PKG_SEARCH_MODULE(FREETYPE REQUIRED freetype2)

find_package(Bullet REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS}
                    ${EPOXY_INCLUDE_DIRS}
                    ${ASSIMP_INCLUDE_DIRS}
                    ${SDL2_IMAGE_INCLUDE_DIRS}
                    ${BULLET_INCLUDE_DIRS}
                    ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(nightmare
                      ${SDL2_LIBRARIES}
                      ${EPOXY_LIBRARIES}
                      ${ASSIMP_LIBRARIES}
                      ${SDL2_IMAGE_LIBRARIES}
                      ${BULLET_LIBRARIES}
                      ${FREETYPE_LIBRARIES}
                      ${LIBCONFIG_LIBRARIES}
                      NIGHTMARE)

# the following is based on
# http://www.cmake.org/Wiki/CMake/Tutorials/Object_Library
# http://neyasystems.com/an-engineers-guide-to-unit-testing-cmake-and-boost-unit-tests/
# http://isti.bitbucket.org/2012/06/01/cmake-check.html

# this enables `make test`
# please note that you will still have to run make manually first
# http://stackoverflow.com/questions/733475/cmake-ctest-make-test-doesnt-build-tests
enable_testing()
file(GLOB TESTS_SRCS t/*.cc)

# for each testing source
foreach(test_src ${TESTS_SRCS})

        # get file name without extension (NAME_WE)
        get_filename_component(test_name ${test_src} NAME_WE)

        add_executable(${test_name} ${test_src})

        # link to our libs
        target_link_libraries(${test_name} NIGHTMARE)

        # move into test_bin
        set_target_properties(${test_name} PROPERTIES 
            RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/test_bin)

        # add it as a test
        add_test(NAME ${test_name}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test_bin 
                 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test_bin/${test_name} )

endforeach(test_src)

