cmake_minimum_required(VERSION 2.8)

project(audio_heatmap)
add_executable(${PROJECT_NAME}
    "main.c"
    "visualization.c"
    "visualization.h"
    "texture-buffer.h"
    "texture-buffer.c"
    "gradient.h"
    "gradient.c"
    )

add_definitions(-Wall -pedantic)

find_package(PkgConfig REQUIRED)

pkg_search_module (CLUTTER REQUIRED clutter-1.0)

pkg_search_module (GSTREAMER REQUIRED gstreamer-1.0)

include_directories(${PROJECT_NAME} PUBLIC
    ${CLUTTER_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    )

target_link_libraries(${PROJECT_NAME}
    ${CLUTTER_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    m
    )

target_compile_options(${PROJECT_NAME} PUBLIC
    ${CLUTTER_CFLAGS_OTHER}
    ${GSTREAMER_CFLAGS_OTHER}
    )

