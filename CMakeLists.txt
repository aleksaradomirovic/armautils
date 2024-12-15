# Copyright (c) 2024  Aleksa Radomirovic
# Licensed under the MIT license. See LICENSE.txt in the project's root directory for details.

cmake_minimum_required(VERSION 3.25)
project(armautils LANGUAGES C)

add_compile_options(
    -std=gnu17
    -O2
    -Wall -Wextra
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(
        -g -fanalyzer
        -march=native
        -mtune=native
    )
endif()

add_compile_definitions(
    PROJECT_VERSION="$<IF:$<BOOL:${CMAKE_PROJECT_VERSION}>,v${CMAKE_PROJECT_VERSION},experimental>"
)

add_subdirectory(pbo)
