# Copyright (c) 2024  Aleksa Radomirovic
# Licensed under the MIT license. See LICENSE.txt in the project's root directory for details.

add_library(
    libpbo SHARED
    src/create.c
    src/extract.c
    src/list.c
    src/utils.c
    src/utils.h
)
target_include_directories(
    libpbo PUBLIC
    include
)
set_property(
    TARGET libpbo
    PROPERTY OUTPUT_NAME pbo
)

add_executable(
    pbo
    src/main.c
)
target_link_libraries(
    pbo PRIVATE
    libpbo
)

install(TARGETS libpbo pbo)
