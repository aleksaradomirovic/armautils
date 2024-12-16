/*
 * Copyright (c) 2024  Aleksa Radomirovic
 * Licensed under the MIT license. See LICENSE.txt in the project's root directory for details.
 */
#include "utils.h"

#include <dirent.h>
#include <endian.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char * fgetstr(FILE * f) {
    char * str = NULL;
    for(size_t i = 0, sz = 0;; i++) {
        if(i == sz) {
            if(__builtin_add_overflow(sz, 16, &sz)) {
                free(str);
                errno = EOVERFLOW;
                return NULL;
            }

            char * nstr = realloc(str, sz);
            if(nstr == NULL) {
                int err = errno;
                free(str);
                errno = err;
                return NULL;
            }
            str = nstr;
        }

        int c = fgetc(f);
        if(c == EOF) {
            free(str);
            errno = EIO;
            return NULL;
        }

        str[i] = (unsigned char) c;
        if(str[i] == '\0') return str;
    }
}

int pbo_to_host_path(char * path) {
#if !defined(_WIN32)
    for(char * sep = strchr(path, '\\'); sep != NULL; sep = strchr(sep, '\\')) {
        *sep++ = SEPARATOR_CHAR;
    }
#endif
    return 0;
}

int host_to_pbo_path(char * path) {
#if !defined(_WIN32)
    for(char * sep = strchr(path, SEPARATOR_CHAR); sep != NULL; sep = strchr(sep, SEPARATOR_CHAR)) {
        *sep++ = '\\';
    }
#endif
    return 0;
}

static void free_pbo_property(struct pbo_property * prop) {
    free(prop->field);
    free(prop->value);
    free(prop);
}

static void free_pbo_property_list(struct pbo_property * list) {
    while(list != NULL) {
        struct pbo_property * next = list->next;
        free_pbo_property(list);
        list = next;
    }
}

static void free_pbo_entry(struct pbo_entry * ent) {
    free(ent->path);
    if(ent->type == PBO_ENTRY_TYPE_VERS) {
        free_pbo_property_list(ent->properties);
    }
    free(ent);
}

static void free_pbo_entry_list(struct pbo_entry * list) {
    while(list != NULL) {
        struct pbo_entry * next = list->next;
        free_pbo_entry(list);
        list = next;
    }
}

void free_pbo_header(struct pbo_entry * list) {
    free_pbo_entry_list(list);
}

static struct pbo_property * read_pbo_property(FILE * pbo) {
    struct pbo_property * prop = malloc(sizeof(struct pbo_property));
    if(prop == NULL) {
        return NULL;
    }

    prop->next = NULL;

    prop->field = fgetstr(pbo);
    if(prop->field == NULL) {
        int err = errno;
        free(prop);
        errno = err;
        return NULL;
    }

    if(strlen(prop->field) == 0) { // end of properties
        free(prop->field);
        prop->field = NULL;
        prop->value = NULL;
        return prop;
    }

    prop->value = fgetstr(pbo);
    if(prop->value == NULL) {
        int err = errno;
        free(prop->field);
        free(prop);
        errno = err;
        return NULL;
    }

    return prop;
}

static struct pbo_property * read_pbo_property_list(FILE * pbo) {
    struct pbo_property * list = NULL;
    for(struct pbo_property * curr = list;;) {
        struct pbo_property * next = read_pbo_property(pbo);
        if(next == NULL) {
            int err = errno;
            free_pbo_property_list(list);
            errno = err;
            return NULL;
        }

        if(next->field == NULL) { // end of properties
            free_pbo_property(next);
            return list;
        }

        if(curr == NULL) {
            list = next;
        } else {
            curr->next = next;
        }
        curr = next;
    }
}

static struct pbo_entry * read_pbo_entry(FILE * pbo) {
    struct pbo_entry * ent = malloc(sizeof(struct pbo_entry));
    if(ent == NULL) {
        return NULL;
    }

    ent->next = NULL;

    ent->path = fgetstr(pbo);
    if(ent->path == NULL) {
        int err = errno;
        free(ent);
        errno = err;
        return NULL;
    }
    if(pbo_to_host_path(ent->path) != 0) {
        free(ent);
        errno = EINVAL;
        return NULL;
    }

    {
        _Static_assert(sizeof(uint32_t) == 4);
        _Static_assert(CHAR_BIT == 8);

        uint32_t metadata[5];
        if(fread(&metadata, 4, 5, pbo) != 5) {
            free(ent->path);
            free(ent);
            errno = EIO;
            return NULL;
        }

        ent->type          = le32toh(metadata[0]);
        ent->original_size = le32toh(metadata[1]);
        ent->offset        = le32toh(metadata[2]);
        ent->timestamp     = le32toh(metadata[3]);
        ent->data_size     = le32toh(metadata[4]);
    }

    if(ent->type == PBO_ENTRY_TYPE_VERS) {
        ent->properties = read_pbo_property_list(pbo);
        if(ent->properties == NULL) {
            int err = errno;
            free(ent->path);
            free(ent);
            errno = err;
            return NULL;
        }
    }

    return ent;
}

static struct pbo_entry * read_pbo_entry_list(FILE * pbo) {
    struct pbo_entry * list = NULL;
    for(struct pbo_entry * curr = list;;) {
        struct pbo_entry * next = read_pbo_entry(pbo);
        if(next == NULL) {
            int err = errno;
            free_pbo_entry_list(list);
            errno = err;
            return NULL;
        }

        if(next->type == PBO_ENTRY_TYPE_NULL && strlen(next->path) == 0) { // end of entries
            free_pbo_entry(next);
            return list;
        }

        if(curr == NULL) {
            list = next;
        } else {
            curr->next = next;
        }
        curr = next;
    }
}

struct pbo_entry * read_pbo_header(FILE * pbo) {
    return read_pbo_entry_list(pbo);
}

static struct pbo_entry * create_pbo_entry(const char * dir, const char * entpath);

static struct pbo_entry * create_pbo_entry_dir(char * dirpath) {
    struct pbo_entry * list = NULL;

    DIR * dir = opendir(dirpath);
    if(dir == NULL) {
        return NULL;
    }
    for(struct pbo_entry * curr = list;;) {
        errno = 0;
        struct dirent * ent = readdir(dir);
        if(ent == NULL) {
            if(errno == 0) {
                if(closedir(dir) != 0) {
                    int err = errno;
                    free_pbo_entry_list(list);
                    errno = err;
                    return NULL;
                }
                return list;
            } else {
                int err = errno;
                free_pbo_entry_list(list);
                closedir(dir);
                errno = err;
                return NULL;
            }
        }

        if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

        struct pbo_entry * next = create_pbo_entry(dirpath, ent->d_name);
        if(next == NULL) {
            if(errno != 0) {
                int err = errno;
                free_pbo_entry_list(list);
                closedir(dir);
                errno = err;
                return NULL;
            } else {
                continue;
            }
        }

        if(curr == NULL) {
            list = next;
        } else {
            curr->next = next;
        }
        curr = next;
        while(curr->next != NULL) {
            curr = curr->next;
        }
    }
}

static struct pbo_entry * create_pbo_entry_file(char * path, const struct stat * info) {
    if(info->st_size > UINT32_MAX) {
        errno = EOVERFLOW;
        return NULL;
    }

    struct pbo_entry * ent = malloc(sizeof(struct pbo_entry));
    if(ent == NULL) {
        return NULL;
    }

    ent->next = NULL;
    ent->path = path;

    ent->type = PBO_ENTRY_TYPE_NULL;
    ent->original_size = info->st_size;
    ent->timestamp = info->st_mtime;

    return ent;
}

static struct pbo_entry * create_pbo_entry(const char * dir, const char * entpath) {
    char * path;
    if(entpath != NULL) {
        path = malloc(strlen(dir) + strlen(entpath) + 2);
        if(path == NULL) {
            return NULL;
        }
        strcpy(path, dir);
        strcat(path, SEPARATOR_STR);
        strcat(path, entpath);
    } else {
        path = strdup(dir);
        if(path == NULL) {
            return NULL;
        }
    }

    struct stat info;
    if(stat(path, &info) != 0) {
        int err = errno;
        free(path);
        errno = err;
        return NULL;
    }

    struct pbo_entry * ent;
    if(S_ISREG(info.st_mode)) {
        ent = create_pbo_entry_file(path, &info);
        if(ent == NULL) {
            int err = errno;
            free(path);
            errno = err;
            return NULL;
        } else {
            return ent;
        }
    } else if(S_ISDIR(info.st_mode)) {
        ent = create_pbo_entry_dir(path);
        if(ent == NULL && errno != 0) {
            int err = errno;
            free(path);
            errno = err;
            return NULL;
        } else {
            free(path);
            errno = 0;
            return ent;
        }
    } else {
        free(path);
        errno = EINVAL;
        return NULL;
    }
}

struct pbo_entry * create_pbo_header(const char * path) {
    return create_pbo_entry(path, NULL);
}

#define COPY_BUF_SZ 0x2000

int copy_file_data(FILE * pbo, const struct pbo_entry * ent, FILE * exf) {
    uintmax_t left = ent->data_size;

    void * copy_buf = malloc(COPY_BUF_SZ);
    if(copy_buf == NULL) {
        return -1;
    }

    while(left > 0) {
        size_t len = (left > COPY_BUF_SZ) ? COPY_BUF_SZ : left;

        if(fread(copy_buf, 1, len, pbo) != len) {
            free(copy_buf);
            errno = EIO;
            return -1;
        }

        if(fwrite(copy_buf, 1, len, exf) != len) {
            free(copy_buf);
            errno = EIO;
            return -1;
        }

        left -= len;
    }

    free(copy_buf);
    return 0;
}

int write_pbo_header(const struct pbo_entry * ent, FILE * pbo) {
    if(fprintf(pbo, "%s", ent->path) < 0 || fputc('\0', pbo) == EOF) {
        errno = EIO;
        return -1;
    }

    {
        uint32_t metadata[5];
        metadata[0] = htole32(ent->type);
        metadata[1] = htole32(ent->original_size);
        metadata[2] = htole32(ent->offset);
        metadata[3] = htole32(ent->timestamp);
        metadata[4] = htole32(ent->data_size);

        if(fwrite(metadata, 4, 5, pbo) != 5) {
            errno = EIO;
            return -1;
        }
    }

    if(ent->type == PBO_ENTRY_TYPE_VERS) {
        for(struct pbo_property * prop = ent->properties; prop != NULL; prop = prop->next) {
            if(fprintf(pbo, "%s", prop->field) < 0 || fputc('\0', pbo) == EOF) {
                errno = EIO;
                return -1;
            }
            if(fprintf(pbo, "%s", prop->value) < 0 || fputc('\0', pbo) == EOF) {
                errno = EIO;
                return -1;
            }
        }
    }

    return 0;
}
