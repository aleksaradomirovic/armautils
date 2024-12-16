/*
 * Copyright (c) 2024  Aleksa Radomirovic
 * Licensed under the MIT license. See LICENSE.txt in the project's root directory for details.
 */
#include "pbo.h"

#include "utils.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <utime.h>

static int mkdir_parents(char * dirpath, const char * path) {
    for(char * sep = strchr(dirpath + strlen(path) + 1, SEPARATOR_CHAR); sep != NULL; sep = strchr(sep, SEPARATOR_CHAR)) {
        *sep = '\0';
        if(mkdir(dirpath, 0777) != 0 && errno != EEXIST) {
            return -1;
        }
        *sep++ = SEPARATOR_CHAR;
    }

    return 0;
}

static int extract_pbo_entry(FILE * pbo, const struct pbo_entry * ent, const char * path, bool preserve_timestamp) {
    char dirpath[strlen(path) + strlen(ent->path) + 2];
    strcpy(dirpath, path);
    strcat(dirpath, SEPARATOR_STR);
    strcat(dirpath, ent->path);

    if(mkdir_parents(dirpath, path) != 0) {
        return -1;
    }

    FILE * exf = fopen(dirpath, "wb");
    if(exf == NULL) {
        return -1;
    }

    if(copy_file_data(pbo, ent, exf) != 0) {
        int err = errno;
        fclose(exf);
        errno = err;
        return -1;
    }

    if(fclose(exf) != 0) {
        return -1;
    }

    if(preserve_timestamp) {        
        struct utimbuf ts = {
            ent->timestamp,
            ent->timestamp,
        };

        if(utime(dirpath, &ts) != 0) {
            return -1;
        }
    }

    return 0;
}

int pbo_extract(FILE * pbo, const char * path, unsigned long flags) {
    struct pbo_entry * header = read_pbo_header(pbo);
    if(header == NULL) {
        return -1;
    }

    for(struct pbo_entry * curr = header; curr != NULL; curr = curr->next) {
        switch(curr->type) {
            case PBO_ENTRY_TYPE_NULL:
                if(extract_pbo_entry(pbo, curr, path, (flags & PBO_TIMESTAMP) != 0) != 0) {
                    int err = errno;
                    free_pbo_header(header);
                    errno = err;
                    return -1;
                }
                break;
            case PBO_ENTRY_TYPE_VERS:
                break;
            default: continue;
        }
    }

    free_pbo_header(header);
    return 0;
}
