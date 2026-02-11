cmake_minimum_required(VERSION 3.10)
project(sdl_graphs C)

# 1. Paths for SDL3 (unchanged)
if(NOT SDL3_DIR)
    set(SDL3_DIR $ENV{SDL3_DIR} CACHE PATH "Path to SDL3 root directory")
endif()
if(NOT SDL3_TTF_DIR)
    set(SDL3_TTF_DIR $ENV{SDL3_TTF_DIR} CACHE PATH "Path to SDL3_ttf root directory")
endif()
if(NOT SDL3_IMAGE_DIR)
    set(SDL3_IMAGE_DIR $ENV{SDL3_IMAGE_DIR} CACHE PATH "Path to SDL3_image root directory")
endif()
# 2. Setup Include Paths
# We add "include" to the list so #include "sdl_graphs.h" works from anywhere
include_directories(
    "include" 
    "${SDL3_DIR}/include" 
    "${SDL3_TTF_DIR}/include"
    "${SDL3_IMAGE_DIR}/include"
)
link_directories("${SDL3_DIR}/lib" "${SDL3_TTF_DIR}/lib" "${SDL3_IMAGE_DIR}/lib")

# --- THE LIBRARY PART ---

# 3. Reference the new 'src' path
add_library(sdl_graphs_lib STATIC src/sdl_graphs.c src/sdl_toolbar.c)
target_link_libraries(sdl_graphs_lib SDL3 SDL3_ttf SDL3_image m)

# --- THE EXAMPLES PART ---

function(add_graph_example name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} sdl_graphs_lib)
    
    if(WIN32)
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL3_DIR}/bin/SDL3.dll"
            "${SDL3_TTF_DIR}/bin/SDL3_ttf.dll"
            "${SDL3_IMAGE_DIR}/bin/SDL3_image.dll"
            $<TARGET_FILE_DIR:${name}>)
    endif()

    # 4. Copy font from the 'assets' folder
    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/PTC55F.ttf"
        $<TARGET_FILE_DIR:${name}>)
endfunction()

# 5. Build your examples
add_graph_example(demo_multi_subplot examples/multi_subplot.c)
add_graph_example(demo_main examples/main.c)
add_graph_example(demo_scatter_plot examples/scatter_plot.c)
add_graph_example(demo_simple examples/simple.c)
add_graph_example(demo_grid examples/grid.c)
add_graph_example(demo_toolbar examples/simple_toolbar.c)
add_graph_example(demo_toolbar_multi examples/multi_toolbar.c)
add_graph_example(demo_multi_subplot_w_toolbar examples/multi_subplot_w_toolbar.c)
add_graph_example(demo_simple3d examples/simple3d.c)
