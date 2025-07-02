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

#pragma once

#include <time.h>

#include "../pbo.h"

enum pbo_entry_type {
    PBO_ENTRY_NULL,
    PBO_ENTRY_VERS,
};

struct pbo_entry {
    char *path;

    enum pbo_entry_type type;

    long original_size;
    long offset;
    time_t timestamp;
    long data_size;

    struct pbo_entry *next;
};

struct pbo_property {
    char *key, *value;

    struct pbo_property *next;
};

struct pbo {
    struct pbo_entry *entries;
    struct pbo_property *properties;
};

int pbo_entry_init(struct pbo_entry **ent);
int pbo_entry_free(struct pbo_entry *ent);

int pbo_property_init(struct pbo_property **prop);
int pbo_property_free(struct pbo_property *prop);
