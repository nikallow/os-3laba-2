cmake_minimum_required(VERSION 3.21)

project(lab2 C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_definitions(_POSIX_C_SOURCE=200809L)

add_executable(parent parent.c)
add_executable(child child.c)

target_compile_options(parent PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(child PRIVATE -Wall -Wextra -Wpedantic)

add_dependencies(parent child)
target_compile_definitions(parent PRIVATE CHILD_PATH="$<TARGET_FILE:child>")
