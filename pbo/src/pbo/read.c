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

#include <endian.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "pbofile.h"

static int fgetasciiz(FILE *file, char *buf, size_t len) {
    for(size_t i = 0; i < len; i++) {
        int c = fgetc(file);
        if(c == EOF) {
            return EIO;
        }

        buf[i] = (char) c;
        if(c == '\0') {
            return 0;
        }
    }
    
    return EOVERFLOW;
}

static int pbo_load_property(struct pbo_property *prop, FILE *file) {
    int status;

    char keybuf[32], valbuf[256];
    status = fgetasciiz(file, keybuf, sizeof(keybuf));
    if(status != 0) {
        return status;
    }

    if(strlen(keybuf) == 0) {
        prop->key = NULL;
        prop->value = NULL;
        return 0;
    }

    status = fgetasciiz(file, valbuf, sizeof(valbuf));
    if(status != 0) {
        return status;
    }

    prop->key = strdup(keybuf);
    if(prop->key == NULL) {
        return errno;
    }

    prop->value = strdup(valbuf);
    if(prop->value == NULL) {
        status = errno;
        free(prop->key);
        return status;
    }

    return 0;
}

static int pbo_load_properties(struct pbo *pbo, FILE *file) {
    int status;

    struct pbo_property **prop_ptr = &pbo->properties;
    while(1) {
        struct pbo_property *prop = NULL;
        status = pbo_property_init(&prop);
        if(status != 0) {
            return status;
        }

        status = pbo_load_property(prop, file);
        if(status != 0) {
            pbo_property_free(prop);
            return status;
        }

        if(prop->key == NULL) {
            status = pbo_property_free(prop);
            if(status != 0) {
                return status;
            }

            break;
        }

        *prop_ptr = prop;
        prop_ptr = &prop->next;
    }

    return 0;
}

static int pbo_load_entry(struct pbo_entry *ent, FILE *file) {
    int status;

    char pathbuf[PATH_MAX];
    status = fgetasciiz(file, pathbuf, PATH_MAX);
    if(status != 0) {
        return status != EOVERFLOW ? status : ENAMETOOLONG;
    }

    uint32_t fields[5];
    if(fread(fields, sizeof(fields), 1, file) != 1) {
        return EIO;
    }

    for(size_t i = 1; i < 5; i++) {
        fields[i] = le32toh(fields[i]);
    }

    if(memcmp(&fields[0], "\0\0\0\0", 4) == 0) {
        ent->type = PBO_ENTRY_NULL;
    } else if(memcmp(&fields[0], "sreV", 4) == 0) {
        ent->type = PBO_ENTRY_VERS;
    } else {
        return EINVAL;
    }

    if( __builtin_add_overflow(fields[1], 0, &ent->original_size) ||
        __builtin_add_overflow(fields[2], 0, &ent->offset)        ||
        __builtin_add_overflow(fields[3], 0, &ent->timestamp)     ||
        __builtin_add_overflow(fields[4], 0, &ent->data_size)) {

        return EOVERFLOW;
    }

    if(strlen(pathbuf) > 0) {
        ent->path = strdup(pathbuf);
        if(ent->path == NULL) {
            return errno;
        }
    } else{
        ent->path = NULL;
    }

    return 0;
}

static int pbo_load_entries(struct pbo *pbo, FILE *file) {
    int status;

    struct pbo_entry **ent_ptr = &pbo->entries;
    while(1) {
        struct pbo_entry *ent = NULL;
        status = pbo_entry_init(&ent);
        if(status != 0) {
            return status;
        }

        status = pbo_load_entry(ent, file);
        if(status != 0) {
            pbo_entry_free(ent);
            return status;
        }

        if(ent->type == PBO_ENTRY_NULL && ent->path == NULL) {
            status = pbo_entry_free(ent);
            if(status != 0) {
                return status;
            }

            break;
        }

        if(ent->type == PBO_ENTRY_VERS) {
            if(ent_ptr != &pbo->entries || ent->path != NULL) {
                pbo_entry_free(ent);
                return EINVAL; // not first in header or non-null path
            }
            
            status = pbo_entry_free(ent);
            if(status != 0) {
                return status;
            }

            status = pbo_load_properties(pbo, file);
            if(status != 0) {
                return status;
            }

            continue;
        }

        *ent_ptr = ent;
        ent_ptr = &ent->next;
    }

    long datapos = ftell(file);
    if(datapos < 0) {
        return errno;
    }

    for(struct pbo_entry *ent = pbo->entries; ent != NULL; ent = ent->next) {
        if(ent->offset == 0) {
            ent->offset = datapos;
        } else {
            datapos = ent->offset;
        }
        datapos += ent->data_size;

        if(ent->original_size == 0) {
            switch(ent->type) {
                case PBO_ENTRY_NULL:
                    ent->original_size = ent->data_size;
                    break;
                default:
                    return ENOTSUP;
            }
        }
    }

    return 0;
}

int pbo_load(struct pbo *pbo, FILE *file) {
    int status;

    status = pbo_load_entries(pbo, file);
    if(status != 0) {
        return status;
    }

    return 0;
}

static int fcopy(FILE *in, FILE *out, long len) {
    if(len < 0) {
        return EINVAL;
    }

    long total = 0;
    char iobuf[BUFSIZ];
    while(total < len) {
        long maxrlen = len - total;
        size_t rlen = maxrlen > BUFSIZ ? BUFSIZ : maxrlen;
        rlen = fread(iobuf, 1, rlen, in);
        if(rlen == 0) {
            return EIO;
        }

        size_t wlen = fwrite(iobuf, 1, rlen, out);
        if(wlen < rlen) {
            return EIO;
        }

        total += wlen;
    }

    return 0;
}

static int pbo_entry_extract_regular(struct pbo_entry *ent, FILE *pbofile) {
    int status;

    char pathbuf[PATH_MAX];
    if(stpncpy(pathbuf, ent->path, PATH_MAX) >= (pathbuf + PATH_MAX)) {
        return ENAMETOOLONG;
    }

    for(char *sep = strstr(pathbuf, PBO_PATH_SEPARATOR); sep != NULL; sep = strstr(++sep, PBO_PATH_SEPARATOR)) {
        *sep = '\0';

        struct stat dirinfo;
        if(stat(pathbuf, &dirinfo) != 0) {
            if(errno == ENOENT) {
                if(mkdir(pathbuf, 00777) != 0) {
                    return errno;
                }
            } else {
                return errno;
            }
        } else if(!S_ISDIR(dirinfo.st_mode)) {
            return ENOTDIR;
        }

        memcpy(sep, "/", strlen("/"));
    }

    FILE *outfile = fopen(pathbuf, "w");
    if(outfile == NULL) {
        return errno;
    }

    if(fseek(pbofile, ent->offset, SEEK_SET) != 0) {
        status = errno;
        fclose(outfile);
        return status;
    }

    status = fcopy(pbofile, outfile, ent->data_size);
    if(status != 0) {
        fclose(outfile);
        return status;
    }

    if(fclose(outfile) != 0) {
        return errno;
    }

    return 0;
}

int pbo_entry_extract(struct pbo_entry *ent, FILE *pbofile) {
    switch(ent->type) {
        case PBO_ENTRY_NULL:
            return pbo_entry_extract_regular(ent, pbofile);
        default:
            return ENOTSUP;
    }
}
