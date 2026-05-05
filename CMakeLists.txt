cmake_minimum_required(VERSION 3.15)
project(sam C)

set(CMAKE_C_STANDARD 11)

set(SOURCES
    src/reciter.c
    src/sam.c
    src/render.c
    src/main.c
    src/debug.c
)

add_executable(sam ${SOURCES})

target_compile_options(sam PRIVATE -Wall -Os)

# Try to find SDL (optional)
find_package(SDL QUIET)

if(SDL_FOUND)
    message(STATUS "SDL found — building with audio support")
    target_compile_definitions(sam PRIVATE USESDL)
    target_include_directories(sam PRIVATE ${SDL_INCLUDE_DIRS})
    target_link_libraries(sam PRIVATE ${SDL_LIBRARIES})
else()
    message(STATUS "SDL not found — building without audio support")
endif()

