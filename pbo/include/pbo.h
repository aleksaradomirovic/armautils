/*
 * Copyright (c) 2024  Aleksa Radomirovic
 * Licensed under the MIT license. See LICENSE.txt in the project's root directory for details.
 */
#pragma once

#include <stdio.h>

int pbo_create(size_t pathc, const char * pathv[], FILE * pbo, unsigned long flags);
int pbo_extract(FILE * pbo, const char * path, unsigned long flags);
int pbo_list(FILE * pbo, FILE * print);

#define PBO_COPYRIGHT_CONTRIBUTORS "Aleksa Radomirovic"
#define PBO_COPYRIGHT_YEARS "2024"
