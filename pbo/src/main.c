/*
 * Copyright (c) 2024  Aleksa Radomirovic
 * Licensed under the MIT license. See LICENSE.txt in the project's root directory for details.
 */
#include "pbo.h"

#include <argp.h>
#include <errno.h>
#include <error.h>

enum {
    MODE_NONE,
    MODE_LIST,
    MODE_EXTRACT,
    MODE_CREATE,
} mode = MODE_NONE;

static const char * pbo_name = NULL;
static int filec;
static char ** filev;

static const struct argp_option args_opts[] = {
    // OPERATING MODES
    { NULL, 0, NULL, 0, "Operating modes:\n", 1 },
    { "list", 't', NULL, 0, "List files in PBO", 0 },
    { "extract", 'x', NULL, 0, "Extract files in PBO", 0 },
    { "create", 'c', NULL, 0, "Create new PBO", 0 },

    // COMMON OPTIONS
    { NULL, 0, NULL, 0, "Common options:\n", 2 },
    { "file", 'f', "PBO", 0, "Specify PBO filename", 0 },

    { NULL, 0, NULL, 0, "Miscellaneous Options:\n", -1 },
    { 0 }
};

static int args_parser(int key, char * arg, struct argp_state *) {
    switch(key) {
        case 't':
            mode = MODE_LIST;
            break;
        case 'x':
            mode = MODE_EXTRACT;
            break;
        case 'c':
            mode = MODE_CREATE;
            break;
        case 'f':
            pbo_name = arg;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

#define VERSION_STR \
"armautils pbo, " PROJECT_VERSION "\n" \
"Copyright (C) " PBO_COPYRIGHT_YEARS "  " PBO_COPYRIGHT_CONTRIBUTORS "\n" \
"Licensed under the MIT license: <https://opensource.org/license/mit>\n" \
"This is free software; you are free to change and redistribute it.\n" \
"There is NO WARRANTY, to the extent permitted by law."

static const struct argp args_info = {
    .options = args_opts,
    .parser = args_parser,
    .args_doc = "[FILE...]",
    .doc = "pbo is a tool for creating, extracting, and manipulating Bohemia Interactive's 'packed bank of files' (PBO) archive format."
};

static void mode_list() {
    if(pbo_name == NULL) {
        error(-1, 0, "No PBO archive specified");
    }

    FILE * archive = fopen(pbo_name, "rb");
    if(archive == NULL) {
        error(-1, errno, "Failed to open archive");
    }

    if(pbo_list(archive, stdout) != 0) {
        int err = errno;
        fclose(archive);
        error(-1, err, "Failed to list archive");
    }

    if(fclose(archive) != 0) {
        error(-1, errno, "Failed to close archive");
    }
}

static void mode_extract() {
    if(pbo_name == NULL) {
        error(-1, 0, "No PBO archive specified");
    }

    FILE * archive = fopen(pbo_name, "rb");
    if(archive == NULL) {
        error(-1, errno, "Failed to open archive");
    }

    if(pbo_extract(archive, ".", 0) != 0) {
        int err = errno;
        fclose(archive);
        error(-1, err, "Failed to extract archive");
    }

    if(fclose(archive) != 0) {
        error(-1, errno, "Failed to close archive");
    }
}

static void mode_create() {
    if(pbo_name == NULL) {
        error(-1, 0, "No PBO archive specified");
    }

    FILE * archive = fopen(pbo_name, "wb");
    if(archive == NULL) {
        error(-1, errno, "Failed to open archive");
    }

    if(pbo_create(filec, (const char **) filev, archive, 0) != 0) {
        int err = errno;
        fclose(archive);
        error(-1, err, "Failed to create archive");
    }

    if(fclose(archive) != 0) {
        error(-1, errno, "Failed to close archive");
    }
}

int main(int argc, char * argv[]) {
    argp_program_version = VERSION_STR;

    int idx;
    if(argp_parse(&args_info, argc, argv, 0, &idx, NULL) != 0) {
        return -1;
    }
    filec = argc - idx;
    filev = argv + idx;

    switch(mode) {
        case MODE_LIST:
            mode_list();
            break;
        case MODE_EXTRACT:
            mode_extract();
            break;
        case MODE_CREATE:
            mode_create();
            break;
        case MODE_NONE:
            error(-1, 0, "No operating mode defined");
            break;
        default:
            error(-1, 0, "Unsupported operating mode");
            break;
    }

    return 0;
}
