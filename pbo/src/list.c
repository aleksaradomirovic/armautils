/*
 * Copyright (c) 2024  Aleksa Radomirovic
 * Licensed under the MIT license. See LICENSE.txt in the project's root directory for details.
 */
#include "pbo.h"

#include "utils.h"

#include <errno.h>

int pbo_list(FILE * pbo, FILE * print) {
    struct pbo_entry * header = read_pbo_header(pbo);
    if(header == NULL) {
        return -1;
    }

    for(struct pbo_entry * curr = header; curr != NULL; curr = curr->next) {
        switch(curr->type) {
            case PBO_ENTRY_TYPE_NULL:
                if(pbo_to_host_path(curr->path) != 0 || fprintf(print, "%s\n", curr->path) < 0) {
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
