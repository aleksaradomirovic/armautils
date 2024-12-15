/*
 * Copyright (c) 2024  Aleksa Radomirovic
 * Licensed under the MIT license. See LICENSE.txt in the project's root directory for details.
 */
#pragma once

#include "pbo.h"

#include <stdint.h>
#include <stdio.h>

#if defined(_WIN32)
#define SEPARATOR_STR "\\"
#define SEPARATOR_CHAR '\\'
#else
#define SEPARATOR_STR "/"
#define SEPARATOR_CHAR '/'
#endif

struct pbo_property {
    char * field;
    char * value;

    struct pbo_property * next;
};

struct pbo_entry {
    char * path;

    uint32_t type;
    uint32_t original_size;
    uint32_t offset;
    uint32_t timestamp;
    uint32_t data_size;

    union {
        struct pbo_property * properties;
    };

    struct pbo_entry * next;
};

#define PBO_ENTRY_TYPE_NULL 0x00000000
#define PBO_ENTRY_TYPE_VERS 0x56657273

void free_pbo_header(struct pbo_entry *);

struct pbo_entry * read_pbo_header(FILE * pbo);
int write_pbo_header(const struct pbo_entry * ent, FILE * pbo);

struct pbo_entry * create_pbo_header(const char * path);

int copy_file_data(FILE * in, const struct pbo_entry * ent, FILE * out);

int pbo_to_host_path(char * path);
int host_to_pbo_path(char * path);
