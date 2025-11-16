cmake_minimum_required(VERSION 3.5)
project(RubberbandUGens)

set(CMAKE_CXX_STANDARD 17)

# Path to SuperCollider source
set(SC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../supercollider" CACHE PATH "Path to SuperCollider source")

# Include SC plugin interface
include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/nova-tt)

# Find Rubberband
find_package(PkgConfig REQUIRED)
pkg_check_modules(RUBBERBAND REQUIRED rubberband)

include_directories(${RUBBERBAND_INCLUDE_DIRS})
link_directories(${RUBBERBAND_LIBRARY_DIRS})

# Build the plugin
add_library(RubberbandUGens MODULE
    source/RubberbandUGens/RubberbandPitchShift.cpp
)

target_link_libraries(RubberbandUGens ${RUBBERBAND_LIBRARIES})

# Set output name
set_target_properties(RubberbandUGens PROPERTIES
    PREFIX ""
    OUTPUT_NAME "RubberbandUGens"
)

# Installation
install(TARGETS RubberbandUGens
    DESTINATION lib/SuperCollider/plugins
)

install(FILES source/RubberbandUGens/RubberbandPitchShift.sc
    DESTINATION share/SuperCollider/Extensions
)
