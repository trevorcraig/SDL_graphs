cmake_minimum_required(VERSION 3.10)
project(sdl_graphs C)

# 1. Provide a way to override paths from the command line or environment
# This checks if a variable was passed; if not, it looks for an environment variable.
if(NOT SDL3_DIR)
    set(SDL3_DIR $ENV{SDL3_DIR} CACHE PATH "Path to SDL3 root directory")
endif()

if(NOT SDL3_TTF_DIR)
    set(SDL3_TTF_DIR $ENV{SDL3_TTF_DIR} CACHE PATH "Path to SDL3_ttf root directory")
endif()

# Error handling if the paths aren't found
if(NOT SDL3_DIR OR NOT SDL3_TTF_DIR)
    message(FATAL_ERROR "SDL3_DIR or SDL3_TTF_DIR not set! Please set them via CMake or Environment Variables.")
endif()

# 2. Include Headers
include_directories("${SDL3_DIR}/include")
include_directories("${SDL3_TTF_DIR}/include")

# 3. Link Directories
link_directories("${SDL3_DIR}/lib")
link_directories("${SDL3_TTF_DIR}/lib")

# 4. Add Source Files
add_executable(sdl_graphs main.c sdl_graphs.c)

# 5. Link Libraries
# On Windows, we often need to link 'm' for math.h and 'mingw32' for entry points
target_link_libraries(sdl_graphs SDL3 SDL3_ttf m)

# 6. Copy DLLs (Dynamic linking fix for Windows)
if(WIN32)
    add_custom_command(TARGET sdl_graphs POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_DIR}/bin/SDL3.dll"
        "${SDL3_TTF_DIR}/bin/SDL3_ttf.dll"
        $<TARGET_FILE_DIR:sdl_graphs>)
endif()

# 7. Copy Font File
add_custom_command(TARGET sdl_graphs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/PTC55F.ttf"
    $<TARGET_FILE_DIR:sdl_graphs>)