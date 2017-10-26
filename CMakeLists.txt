cmake_minimum_required(VERSION 3.5)
project(nightmare)

# from http://www.cmake.org/Wiki/CMake/Tutorials/C%2B%2B11Flags
if(UNIX)
    SET(CMAKE_CXX_FLAGS "-Wall -DGLM_FORCE_RADIANS")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(nightmare main.cc)

# here we compile all of our source files into one library
# this is used below to link both to our main executable
# as well as to link to each of our tests
set(EN_SOURCES
        src/atlas.cc
        src/blob.cc
        src/char.cc
        src/component/component_system_manager.cc
        src/component/door_component.cc
        src/component/gas_production_component.cc
        src/component/light_component.cc
        src/component/physics_component.cc
        src/component/power_component.cc
        src/component/power_provider_component.cc
        src/component/pressure_sensor_component.cc
        src/component/proximity_sensor_component.cc
        src/component/reader_component.cc
        src/component/relative_position_component.cc
        src/component/renderable_component.cc
        src/component/sensor_comparator_component.cc
        src/component/surface_attachment_component.cc
        src/component/switch_component.cc
        src/component/type_component.cc
        src/config.cc
        src/input.cc
        src/mesh.cc
        src/mesher.cc
        src/mock_ship_junk.cc
        src/particle.cc
        src/physics.cc
        src/projectile/projectile.cc
        src/settings.cc
        src/shader.cc
        src/ship_space.cc
        src/sprites.cc
        src/text.cc
        src/textureset.cc
        src/tools/add_block.cc
        src/tools/add_surface.cc
        src/tools/fire_projectile.cc
        src/tools/remove_block.cc
        src/tools/remove_surface.cc
        src/wiring/wiring.cc
        src/wiring/wiring_data.cc
        )

add_library(NIGHTMARE ${EN_SOURCES})

include(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(EPOXY REQUIRED epoxy)
PKG_SEARCH_MODULE(ASSIMP REQUIRED assimp)
PKG_SEARCH_MODULE(SDL2_IMAGE REQUIRED SDL2_image)
PKG_SEARCH_MODULE(FREETYPE REQUIRED freetype2)

find_package(Bullet REQUIRED)

find_path(LIBCONFIG_INCLUDE_DIRS libconfig.h)
find_library(LIBCONFIG_LIBRARIES config)

include_directories(${SDL2_INCLUDE_DIRS}
                    ${EPOXY_INCLUDE_DIRS}
                    ${ASSIMP_INCLUDE_DIRS}
                    ${SDL2_IMAGE_INCLUDE_DIRS}
                    ${BULLET_INCLUDE_DIRS}
                    ${FREETYPE_INCLUDE_DIRS}
                    ${LIBCONFIG_INCLUDE_DIRS})

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

