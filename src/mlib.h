#ifndef MLIB_H
#define MLIB_H 1

#include <stdlib.h>

// TODO maybe we should just expose a function to register a pointer for deletion or register a pointer for deletion with a custom function

// memory lifecycle definition enum
enum lifecycle {
	application,
	daemon_run
};

// checked malloc, if it fails it exits the program
void* c_malloc(size_t size);

// checked realloc, if it fails it frees the old pointer and exits the program
void* c_realloc(void* ptr, size_t size);

void* c_malloc_reg(size_t size, enum lifecycle scope);

void* c_realloc_reg(void* ptr, size_t size, enum lifecycle scope);

void c_free_reg(void* ptr);

void c_free_scope(enum lifecycle scope);

void* reg_ptr(void* ptr, enum lifecycle scope);

void* reg_ptr_func(void* ptr, void (*free_func)(void* ptr), enum lifecycle scope);

#endif