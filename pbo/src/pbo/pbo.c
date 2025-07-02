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
#include <stdlib.h>

#include "pbofile.h"

int pbo_entry_init(struct pbo_entry **ent_ptr) {
    struct pbo_entry *ent = calloc(1, sizeof(struct pbo_entry));
    if(ent == NULL) {
        return errno;
    }

    *ent_ptr = ent;
    return 0;
}

int pbo_entry_free(struct pbo_entry *ent) {
    free(ent->path);
    free(ent);
    return 0;
}

int pbo_property_init(struct pbo_property **prop_ptr) {
    struct pbo_property *prop = calloc(1, sizeof(struct pbo_property));
    if(prop == NULL) {
        return errno;
    }

    *prop_ptr = prop;
    return 0;
}

int pbo_property_free(struct pbo_property *prop) {
    free(prop->key);
    free(prop->value);
    free(prop);
    return 0;
}

int pbo_init(struct pbo **pbo_ptr) {
    struct pbo *pbo = calloc(1, sizeof(struct pbo));
    if(pbo == NULL) {
        return errno;
    }

    *pbo_ptr = pbo;
    return 0;
}

static int pbo_entries_free(struct pbo *pbo) {
    int status;

    for(struct pbo_entry *ent = pbo->entries; ent != NULL;) {
        struct pbo_entry *next = ent->next;
        status = pbo_entry_free(ent);
        if(status != 0) {
            return status;
        }
        ent = next;
    }

    pbo->entries = NULL;
    return 0;
}

static int pbo_properties_free(struct pbo *pbo) {
    int status;

    for(struct pbo_property *prop = pbo->properties; prop != NULL;) {
        struct pbo_property *next = prop->next;
        status = pbo_property_free(prop);
        if(status != 0) {
            return status;
        }
        prop = next;
    }

    pbo->properties = NULL;
    return 0;
}

int pbo_destroy(struct pbo *pbo) {
    int status;

    status = pbo_entries_free(pbo);
    if(status != 0) {
        return status;
    }

    status = pbo_properties_free(pbo);
    if(status != 0) {
        return status;
    }

    free(pbo);
    return 0;
}

/*
 *
 */

const char * pbo_property_key(struct pbo_property *prop) {
    return prop->key;
}

const char * pbo_property_value(struct pbo_property *prop) {
    return prop->value;
}

struct pbo_property * pbo_property_next(struct pbo_property *prop) {
    return prop->next;
}

const char * pbo_entry_path(struct pbo_entry *ent) {
    return ent->path;
}

struct pbo_entry * pbo_entry_next(struct pbo_entry *ent) {
    return ent->next;
}

struct pbo_entry * pbo_get_entries(struct pbo *pbo) {
    return pbo->entries;
}

struct pbo_property * pbo_get_properties(struct pbo *pbo) {
    return pbo->properties;
}
