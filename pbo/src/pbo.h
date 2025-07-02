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

#include <stdio.h>

#define PBO_PATH_MAX 260
#define PBO_PATH_SEPARATOR "\\"

typedef struct pbo_entry PBO_ENTRY;
typedef struct pbo_property PBO_PROPERTY;
typedef struct pbo PBO;

int pbo_init(PBO **pbo);
int pbo_destroy(PBO *pbo);

int pbo_load(PBO *pbo, FILE *file);
int pbo_save(PBO *pbo, FILE *file);

const char * pbo_property_key(PBO_PROPERTY *prop);
const char * pbo_property_value(PBO_PROPERTY *prop);
PBO_PROPERTY * pbo_property_next(PBO_PROPERTY *prop);

const char * pbo_entry_path(PBO_ENTRY *ent);
PBO_ENTRY * pbo_entry_next(PBO_ENTRY *ent);

int pbo_entry_extract(PBO_ENTRY *ent, FILE *pbofile);

PBO_ENTRY * pbo_get_entries(PBO *pbo);
PBO_PROPERTY * pbo_get_properties(PBO *pbo);
