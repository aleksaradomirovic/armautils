/*
 * Copyright 2025 Aleksa Radomirovic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <limits.h>
#include <string.h>

#include "mode.h"
#include "../pbo.h"

int pbo_mode_list(const char *path) {
    int status;

    struct pbo *pbo;
    status = pbo_init(&pbo);
    if(status != 0) {
        return status;
    }

    FILE *file = fopen(path, "r");
    if(file == NULL) {
        status = errno;
        pbo_destroy(pbo);
        return status;
    }

    status = pbo_load(pbo, file);
    if(status != 0) {
        fclose(file);
        pbo_destroy(pbo);
        return status;
    }

    for(PBO_ENTRY *ent = pbo_get_entries(pbo); ent != NULL; ent = pbo_entry_next(ent)) {
        fprintf(stdout, "%s\n", pbo_entry_path(ent));
    }

    if(fclose(file) != 0) {
        status = errno;
        pbo_destroy(pbo);
        return status;
    }

    status = pbo_destroy(pbo);
    if(status != 0) {
        return status;
    }

    return 0;
}
