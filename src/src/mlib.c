#include "mlib.h"

#include <pthread.h>
#include <sysexits.h>

#include "lib/stack.h"
#include "lib/logger.h"

STACK_INIT(allocs, ptr_w_t) // stack type used just as list of ptr_w_t
STACK_INIT(stack, stack_t(allocs)) // stack used as stack of allocations elements [stack_t(allocs)]

static inline void ptr_w_free(ptr_w_t *ptr_w) {
	if(ptr_w->free_func == NULL) {
		free(ptr_w->ptr);
	}
	else {
		ptr_w->free_func(ptr_w->ptr);
	}
}

// this function should be called only with an argumenta allocated on the stack, it does not completely free the stack_t(allocs) object
static inline void allocs_free(stack_t(allocs) *element) {
	for(size_t i = 0; i < element->size; ++i) {
		ptr_w_free(element->array + i);
	}

	stack_free(allocs, element);
}

static void stack_destructor(void *ptr) {
	stack_t(stack) *stack_ptr = (stack_t(stack)*) ptr;
	stack_t(allocs) element;

	while(!stack_empty(stack, stack_ptr)) {
		element = stack_pop(stack, stack_ptr);
		allocs_free(&element);
	}

	stack_destroy(stack, stack_ptr);
}

static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static pthread_key_t key;

static void make_stack_key() {
    (void) pthread_key_create(&key, stack_destructor);
}

static inline stack_t(stack) *get_stack_ptr() {
	stack_t(stack) *stack_ptr;

    (void) pthread_once(&key_once, make_stack_key);

    if ((stack_ptr = pthread_getspecific(key)) == NULL) {
        stack_ptr = stack_init(stack);

        (void) pthread_setspecific(key, stack_ptr);

        // during the first call and after pthread_setspecific push get called to initialize an element on the stack
        push();
    }

    return stack_ptr;
}

void push() {
	stack_t(stack) *stack_ptr = get_stack_ptr();
	stack_t(allocs) new_allocs = stack_init_local(allocs);

	stack_push(stack, stack_ptr, &new_allocs);
}

void pop() {
	stack_t(stack) *stack_ptr = get_stack_ptr();
	// get and remove current stack entry
	stack_t(allocs) allocs_element = stack_pop(stack, stack_ptr);

	allocs_free(&allocs_element);
}

// this function should be called only with an argument allocated on the stack, it does not completely free the stack_t(allocs) object
static inline ptr_w_t allocs_free_complement(stack_t(allocs) *alloc_ptr, void *ptr) {
	ptr_w_t match_ptr_w = { 0 }; // if no ptr wrapper is found a dummy empty one will be returned

	for(size_t i = 0; i < alloc_ptr->size; ++i) {
		ptr_w_t *ptr_w = alloc_ptr->array + i;

		if(ptr_w->ptr != ptr) {
			// free all ptrs except the passed one
			ptr_w_free(alloc_ptr->array + i);
		}
		else {
			// save match and return ptr
			match_ptr_w = *ptr_w;
		}
	}

	stack_free(allocs, alloc_ptr);

	return match_ptr_w;
}

void *pop_trn(void* ptr) {
	stack_t(stack) *stack_ptr = get_stack_ptr();
	// get and remove current stack entry
	stack_t(allocs) allocs_element = stack_pop(stack, stack_ptr);
	// found pointer wrapper
	ptr_w_t match_ptr_w = allocs_free_complement(&allocs_element, ptr);

	return match_ptr_w.ptr;
}

ptr_w_t pop_trn_w(void* ptr) {
	stack_t(stack) *stack_ptr = get_stack_ptr();
	// get and remove current stack entry
	stack_t(allocs) allocs_element = stack_pop(stack, stack_ptr);
	// found pointer wrapper
	ptr_w_t match_ptr_w = allocs_free_complement(&allocs_element, ptr);

	return match_ptr_w;
}

void *reg_ptr(void* ptr) {
	stack_t(stack) *stack_ptr = get_stack_ptr();
	// get current stack entry
	stack_t(allocs) *alloc_ptr = stack_peek(stack, stack_ptr);

	if(alloc_ptr) {
		// add ptr to current stack entry
		ptr_w_t ptr_w = {
			.ptr = ptr,
			.free_func = NULL
		};

		stack_push(allocs, alloc_ptr, &ptr_w);

		return ptr;
	}

	return NULL;
}

void *reg_ptr_fn(void* ptr, void (*free_func)(void*)) {
	stack_t(stack) *stack_ptr = get_stack_ptr();
	// get current stack entry
	stack_t(allocs) *alloc_ptr = stack_peek(stack, stack_ptr);

	if(alloc_ptr) {
		// add ptr to current stack entry
		ptr_w_t ptr_w = {
			.ptr = ptr,
			.free_func = free_func
		};

		stack_push(allocs, alloc_ptr, &ptr_w);

		return ptr;
	}

	return NULL;
}

void *reg_ptr_w(ptr_w_t ptr_w) {
	if(!ptr_w.ptr) {
		return NULL;
	}

	stack_t(stack) *stack_ptr = get_stack_ptr();
	// get current stack entry
	stack_t(allocs) *alloc_ptr = stack_peek(stack, stack_ptr);

	if(alloc_ptr) {
		// add ptr to current stack entry
		stack_push(allocs, alloc_ptr, &ptr_w);

		return ptr_w.ptr;
	}

	return NULL;
}
