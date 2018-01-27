cmake_minimum_required(VERSION 3.2)
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
    src/asset_manager.cc
    src/atlas.cc
    src/bullet_debug_draw.h
    src/blob.cc
    src/char.cc
    src/component/component_system_manager.cc
    src/component/display_component.cc
    src/component/door_component.cc
    src/component/gas_producer_component.cc
    src/component/light_component.cc
    src/component/physics_component.cc
    src/component/placeable_component.cc
    src/component/power_component.cc
    src/component/power_provider_component.cc
    src/component/pressure_sensor_component.cc
    src/component/proximity_sensor_component.cc
    src/component/reader_component.cc
    src/component/position_component.cc
    src/component/renderable_component.cc
    src/component/sensor_comparator_component.cc
    src/component/surface_attachment_component.cc
    src/component/switch_component.cc
    src/component/type_component.cc
    src/component/wire_comms_component.cc
    src/config.cc
    src/input.cc
    src/imgui_impl_sdl_gl3.cc
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
    src/tools/add_entity.cc
    src/tools/cut_wall.cc
    src/tools/paint_surface.cc
    src/tools/fire_projectile.cc
    src/tools/remove_block.cc
    src/tools/remove_surface.cc
    src/tools/remove_entity.cc
    src/tools/wiring.cc
    src/wiring/wiring_data.cc
    src/entity_utils.cc
    src/enums/enums.cc
    src/utils/debugdraw_gl.cc
)

set(EN_HEADERS
    src/asset_manager.h
    src/blob.h
    src/block.h
    src/char.h
    src/chunk.h
    src/common.h
    src/component/c_entity.h
    src/component/component_manager.h
    src/component/component_system_manager.h
    src/component/component_managers.h
    src/component/display_component.h
    src/component/door_component.h
    src/component/gas_producer_component.h
    src/component/light_component.h
    src/component/physics_component.h
    src/component/placeable_component.h
    src/component/power_component.h
    src/component/power_provider_component.h
    src/component/pressure_sensor_component.h
    src/component/proximity_sensor_component.h
    src/component/reader_component.h
    src/component/position_component.h
    src/component/renderable_component.h
    src/component/sensor_comparator_component.h
    src/component/surface_attachment_component.h
    src/component/switch_component.h
    src/component/type_component.h
    src/component/wire_comms_component.h
    src/config.h
    src/fixed_cube.h
    src/input.h
    src/imgui_impl_sdl_gl3.h
    src/libconfig_shim.h
    src/memory.h
    src/mesh.h
    src/particle.h
    src/physics.h
    src/player.h
    src/projectile/projectile.h
    src/render_data.h
    src/scopetimer.h
    src/settings.h
    src/shader.h
    src/ship_space.h
    src/text.h
    src/textureset.h
    src/timer.h
    src/tools/tools.h
    src/winerr.h
    src/winunistd.h
    src/wiring/wiring_data.h
    src/wiring/wiring.h
    src/tinydir.h
    src/entity_utils.h
    src/enums/enums.h
    src/utils/debugdraw.h
)

set(IMGUI_SOURCES
    src/imgui/imconfig.h
    src/imgui/imgui.cpp
    src/imgui/imgui.h
    src/imgui/imgui_demo.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_internal.h
    src/imgui/stb_rect_pack.h
    src/imgui/stb_textedit.h
    src/imgui/stb_truetype.h
)     

option(ALSA "Enable ALSA audio backend" ON)
option(OSS "Enable OSS audio backend" OFF)
option(COREAUDIO "Enable Core Audio audio backend" OFF)
option(NULL "Enable NULL audio backend" ON)

if(ALSA)
    add_definitions(-DWITH_ALSA)
endif(ALSA)

if(OSS)
    add_definitions(-DWITH_OSS)
endif(OSS)

if(COREAUDIO)
    add_definitions(-DWITH_COREAUDIO)
endif(COREAUDIO)

if(NULL)
    add_definitions(-DWITH_NULL)
endif(NULL)

set(SOLOUD_SOURCES
    src/soloud/src/audiosource/wav/soloud_wav.cpp
    src/soloud/src/audiosource/wav/soloud_wavstream.cpp
    src/soloud/src/audiosource/wav/stb_vorbis.c
    src/soloud/src/audiosource/wav/stb_vorbis.h
    src/soloud/src/backend/null/soloud_null.cpp
    src/soloud/src/backend/alsa/soloud_alsa.cpp
    src/soloud/src/backend/oss/soloud_oss.cpp
    src/soloud/src/backend/coreaudio/soloud_coreaudio.cpp
    src/soloud/src/core/soloud.cpp
    src/soloud/src/core/soloud_audiosource.cpp
    src/soloud/src/core/soloud_bus.cpp
    src/soloud/src/core/soloud_core_3d.cpp
    src/soloud/src/core/soloud_core_basicops.cpp
    src/soloud/src/core/soloud_core_faderops.cpp
    src/soloud/src/core/soloud_core_filterops.cpp
    src/soloud/src/core/soloud_core_getters.cpp
    src/soloud/src/core/soloud_core_setters.cpp
    src/soloud/src/core/soloud_core_voicegroup.cpp
    src/soloud/src/core/soloud_core_voiceops.cpp
    src/soloud/src/core/soloud_fader.cpp
    src/soloud/src/core/soloud_fft.cpp
    src/soloud/src/core/soloud_fft_lut.cpp
    src/soloud/src/core/soloud_file.cpp
    src/soloud/src/core/soloud_filter.cpp
    src/soloud/src/core/soloud_thread.cpp
    src/soloud/src/filter/soloud_bassboostfilter.cpp
    src/soloud/src/filter/soloud_biquadresonantfilter.cpp
    src/soloud/src/filter/soloud_dcremovalfilter.cpp
    src/soloud/src/filter/soloud_echofilter.cpp
    src/soloud/src/filter/soloud_fftfilter.cpp
    src/soloud/src/filter/soloud_flangerfilter.cpp
    src/soloud/src/filter/soloud_lofifilter.cpp
)

add_library(NIGHTMARE ${EN_SOURCES} ${EN_HEADERS} ${IMGUI_SOURCES} ${SOLOUD_SOURCES})

include(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(EPOXY REQUIRED epoxy)
PKG_SEARCH_MODULE(ASSIMP REQUIRED assimp)
PKG_SEARCH_MODULE(SDL2_IMAGE REQUIRED SDL2_image)
PKG_SEARCH_MODULE(FREETYPE REQUIRED freetype2)

find_package(Bullet REQUIRED)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include(FindALSA)

find_path(LIBCONFIG_INCLUDE_DIRS libconfig.h)
find_library(LIBCONFIG_LIBRARIES config)

include_directories(
    ${ALSA_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${EPOXY_INCLUDE_DIRS}
    ${ASSIMP_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${BULLET_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${LIBCONFIG_INCLUDE_DIRS}
    src/glm
    src/soloud/include
)

target_link_libraries(nightmare
  NIGHTMARE
  ${ALSA_LIBRARY}
  ${SDL2_LIBRARIES}
  ${EPOXY_LIBRARIES}
  ${ASSIMP_LIBRARIES}
  ${SDL2_IMAGE_LIBRARIES}
  ${BULLET_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  ${LIBCONFIG_LIBRARIES}
  Threads::Threads
)

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
    # TODO: uncouple this.
        target_link_libraries(${test_name} NIGHTMARE ${BULLET_LIBRARIES})

        # move into test_bin
        set_target_properties(${test_name} PROPERTIES 
            RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/test_bin)

        # add it as a test
        add_test(NAME ${test_name}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test_bin 
                 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test_bin/${test_name} )

endforeach(test_src)

