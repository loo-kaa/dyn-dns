#include "mlib.h"

#include <string.h>
#include <pthread.h>
#include <sysexits.h>

#include "lib/hashmap.h"
#include "lib/logger.h"

#define INITIAL_MAP_SIZE 10

typedef pthread_t THREAD_T;

// struct that holds variable data
struct allocation {
	enum lifecycle scope;
	THREAD_T tid;
	void* ptr;
	void (*free_func) (void* ptr); // custom free function
};

// used to hold a match allocation and an array of pointers
struct matcher {
	struct allocation* input;
	void** to_delete;
	size_t size;
};

static struct hashmap* map = NULL;

// hashmap compare function
int allocation_compare(const void *a, const void *b, void *udata) {
    const struct allocation *alloc_a = a;
    const struct allocation *alloc_b = b;

    // compares only the pointer since pointers are unique
    return alloc_a->ptr != alloc_b->ptr; // 0 is success
}

// hashmap hash function
uint64_t allocation_hash(const void* item, uint64_t seed0, uint64_t seed1) {
    const struct allocation* alloc = item;

    // hashes only pointer data since pointers are unique
    return hashmap_sip(&alloc->ptr, sizeof(void*), seed0, seed1);
}

static inline void allocation_free(const struct allocation* alloc) {
	if(alloc->free_func == NULL) {
		free(alloc->ptr);
	}
	else {
		alloc->free_func(alloc->ptr);
	}
}

// hashmap itaration function
bool allocation_iter_free(const void* item, void* udata) {
	allocation_free((const struct allocation*) item);
    return true;
}

// function that frees every pointer in the map and the map itself it should be called only before the end of the program 
void free_map() {
	hashmap_scan(map, allocation_iter_free, NULL);
	hashmap_free(map);
}

// constructor called before the main to initialize the structure that holds the allocation data
void __attribute__ ((constructor)) setup() {
	map = hashmap_new(sizeof(struct allocation), INITIAL_MAP_SIZE, 0, 0, allocation_hash, allocation_compare, NULL);

	if(map == NULL) {
		log_error("Memory map allocation error");
		exit(EX_OSERR);
	}

	atexit(free_map);
}

void* c_malloc(size_t size) {
	void* ptr = malloc(size);

	if(ptr == NULL) {
		log_error("Memory allocation error");
		exit(EX_OSERR);
	}

	return ptr;
}

void* c_realloc(void* ptr, size_t size) {
	void* newPtr = realloc(ptr, size);

	if(newPtr == NULL) {
		log_error("Memory reallocation error");
		free(ptr);
		exit(EX_OSERR);
	}

	return newPtr;
}

void* c_malloc_reg(size_t size, enum lifecycle scope) {
	void* ptr = c_malloc(size);

	hashmap_set(map, &(struct allocation){
		.scope = scope,
		.tid = pthread_self(),
		.ptr = ptr,
		.free_func = NULL });

	return ptr;
}

void* c_realloc_reg(void* ptr, size_t size, enum lifecycle scope) {
	const struct allocation* alloc = hashmap_get(map, &(struct allocation){ .ptr = ptr });

	// if the old allocation for the passed pointer exists we have to remove it from the map
	if(alloc != NULL) {
		hashmap_delete(map, &(struct allocation){ .ptr = ptr });
	}

	void* newPtr = c_realloc(ptr, size);

	hashmap_set(map, &(struct allocation){
		.scope = scope,
		.tid = pthread_self(),
		.ptr = newPtr,
		.free_func = NULL });

	return newPtr;
}

/**
 * Frees memory of a pointer in the map before its lifecycle ends
 * if the pointer is not in the internal map this method won't do anything
 **/
void c_free_reg(void* ptr) {
	const struct allocation* alloc = hashmap_get(map, &(struct allocation){ .ptr = ptr });

	if(alloc != NULL) {
		allocation_free((const struct allocation*) alloc);
		hashmap_delete(map, (void*) alloc);
	}
}

bool allocation_iter_scope_free(const void* item, void* udata) {
	struct matcher* matcher = udata;

	const struct allocation* current_alloc = item;    
    const struct allocation* match_alloc = matcher->input;

	if(current_alloc->scope == match_alloc->scope && pthread_equal(current_alloc->tid, match_alloc->tid)) {
		allocation_free(current_alloc);
		matcher->to_delete[matcher->size++] = current_alloc->ptr;
	}

	return true;
}

/**
 * Frees every variable associated to the passed scope allocated by the calling thread
 **/
void c_free_scope(enum lifecycle scope) {
	void** to_delete = c_malloc(sizeof(void*) * hashmap_count(map));

	struct allocation match_alloc = (struct allocation){
		.scope = scope,
		.tid = pthread_self()
	};

	struct matcher matcher = (struct matcher){
		.input = &match_alloc,
		.to_delete = to_delete,
		.size = 0
	};

	// scan and pass additional data that holds current thread and scope info to match allocations to free
	hashmap_scan(map, allocation_iter_scope_free, &matcher);

	// iterate over updated array and delete map records
	for(size_t i = 0; i < matcher.size; ++i) {
		hashmap_delete(map, &(struct allocation){ .ptr = matcher.to_delete[i] });
	}

	free(to_delete);
}

/**
 * Registers a pointer in the map associated with a scope.
 * The system will free the registered pointer upon closing of the specified scope.
 **/
void* reg_ptr(void* ptr, enum lifecycle scope) {
	if(ptr != NULL) {
		hashmap_set(map, &(struct allocation){
			.scope = scope,
			.tid = pthread_self(),
			.ptr = ptr,
			.free_func = NULL });

		return ptr;
	}

	return NULL;
}

/**
 * Registers a custom pointer in the map associated with a scope.
 * The system will free the registered pointer upon closing of the specified scope.
 * In this case the free function is passed in input by the user
 **/
void* reg_ptr_func(void* ptr, void (*free_func)(void* ptr), enum lifecycle scope) {
	if(ptr != NULL) {
		hashmap_set(map, &(struct allocation){
			.scope = scope,
			.tid = pthread_self(),
			.ptr = ptr,
			.free_func = free_func });

		return ptr;
	}

	return NULL;
}