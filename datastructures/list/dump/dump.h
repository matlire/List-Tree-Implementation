#ifndef DUMP_H
#define DUMP_H

#include "../list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

void list_dump_reset(const char* html_file);

void list_dump(const list_t* list, size_t capacity,
               const char* title, const char* html_file);

#endif

