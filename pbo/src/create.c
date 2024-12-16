/*
 * Copyright (c) 2024  Aleksa Radomirovic
 * Licensed under the MIT license. See LICENSE.txt in the project's root directory for details.
 */
#include "pbo.h"

#include "utils.h"

#include <errno.h>
#include <string.h>

static int pack_pbo_entry(struct pbo_entry * ent, FILE * pbo) {
    FILE * inf = fopen(ent->path, "rb");
    if(inf == NULL) {
        return -1;
    }

    if(copy_file_data(inf, ent, pbo) != 0) {
        int err = errno;
        fclose(inf);
        errno = err;
        return -1;
    }

    if(fclose(inf) != 0) {
        return -1;
    }

    return 0;
}

static void fix_last_separator(char * path) {
    char * lsep = strrchr(path, SEPARATOR_CHAR);
    if(lsep == NULL) return;
    if(*(lsep+1) == '\0') *lsep = '\0';
}

int pbo_create(size_t pathc, const char * pathv[], FILE * pbo, unsigned long flags) {
    if(pathc <= 0) {
        errno = EINVAL;
        return -1;
    }

    struct pbo_entry * header = NULL, * curr = header;
    for(size_t i = 0; i < pathc; i++) {
        char path[strlen(pathv[i]) + 1];
        strcpy(path, pathv[i]);
        fix_last_separator(path);

        struct pbo_entry * sublist = create_pbo_header(path);
        if(sublist == NULL) {
            if(errno != 0) {
                int err = errno;
                free_pbo_header(header);
                errno = err;
                return -1;
            } else {
                continue;
            }
        }

        if(curr == NULL) {
            header = sublist;
        }
        curr = sublist;
        while(curr->next != NULL) {
            curr = curr->next;
        }
    }

    for(curr = header; curr != NULL; curr = curr->next) {
        curr->data_size = curr->original_size;
        if(!(flags & PBO_TIMESTAMP)) {
            curr->timestamp = 0;
        }
        if(write_pbo_header(curr, pbo) != 0) {
            int err = errno;
            free_pbo_header(header);
            errno = err;
            return -1;
        }
    }

    {
        struct pbo_entry null_ent = { 0 };
        null_ent.path = "";
        null_ent.type = PBO_ENTRY_TYPE_NULL;
        null_ent.original_size = 0;
        null_ent.data_size = 0;

        if(write_pbo_header(&null_ent, pbo) != 0) {
            int err = errno;
            free_pbo_header(header);
            errno = err;
            return -1;
        }
    }

    for(curr = header; curr != NULL; curr = curr->next) {
        switch(curr->type) {
            case PBO_ENTRY_TYPE_NULL:
                if(pack_pbo_entry(curr, pbo) != 0) {
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
