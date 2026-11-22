cmake_minimum_required(VERSION 3.16)

project(os_lab4 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(os_core STATIC
    src/line_utils.c
    src/platform/os_linux.c
)

target_include_directories(os_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

target_link_libraries(os_core PUBLIC Threads::Threads)

if(UNIX AND NOT APPLE)
    target_link_libraries(os_core PUBLIC rt)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(os_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(parent src/parent.c)
target_link_libraries(parent PRIVATE os_core)

add_executable(child src/child.c)
target_link_libraries(child PRIVATE os_core)
