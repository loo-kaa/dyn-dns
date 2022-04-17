#ifndef UTILS_H
#define UTILS_H 1

#include <stdio.h>
#include "mlib.h"

struct property {
	char* key;
	char* value;
};

char* read_file(FILE* file_ptr);

char* read_line(FILE* file_ptr);

struct property* read_property_file(FILE* file_ptr);

char* get_property_value(struct property* properties, char* key);

void free_properties(struct property* properties);

char* format_string(char* format, ...);

char* strstr_block(char* str, const char* word, char block);

#endif