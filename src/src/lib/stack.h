#ifndef STACK_H
#define STACK_H

#include <stdlib.h>

/*!
  @header
 
  Generic stack library.

  The basic structure, specifically the use of preprocessor macros is "stolen" from khash.h "https://attractivechaos.wordpress.com/2009/09/29/khash-h/"
  This is not ideal from a ease of reading or debugging standpoint but this data structure should be used only in mlib.c
 
  @copyright Luca A
 */

static inline int min(int x, int y) {
	return (x < y) ? x : y;
}

static inline int max(int x, int y) {
	return (x > y) ? x : y;
}

#define STACK_INIT(name, data_t) \
    typedef struct {\
        size_t size, allocated;\
        data_t *array;\
    } stack_##name##_t;\
    static inline stack_##name##_t *stack_init_##name() {\
        return calloc(1, sizeof(stack_##name##_t));\
    }\
    static inline stack_##name##_t stack_init_local_##name() {\
        return (stack_##name##_t) { 0 };\
    }\
    static inline void stack_destroy_##name(stack_##name##_t *stack) {\
        if (stack) {\
            free(stack->array);\
            free(stack);\
        }\
    }\
    static inline void stack_free_##name(stack_##name##_t *stack) {\
        if (stack) {\
            free(stack->array);\
            stack->size = stack->allocated = 0;\
            stack->array = NULL;\
        }\
    }\
    static inline void stack_clear_##name(stack_##name##_t *stack) {\
        if (stack) {\
            stack->size = 0;\
        }\
    }\
    static inline void stack_resize_##name(stack_##name##_t *stack, size_t new_size) {\
    	new_size = max(new_size, 1);\
        stack->array = realloc(stack->array, sizeof(data_t) * new_size);\
        stack->allocated = new_size;\
        stack->size = min(stack->size, new_size);\
    }\
    static inline data_t stack_pop_##name(stack_##name##_t *stack) {\
        if (stack->size) {\
        	data_t element = stack->array[--stack->size];\
        	size_t half_size = stack->allocated >> 1;\
        	if(stack->size <= half_size) {\
        		stack_resize_##name(stack, half_size);\
        	}\
        	return element;\
        }\
        else {\
        	return (data_t) { 0 };\
        }\
    }\
    static inline void stack_push_##name(stack_##name##_t *stack, data_t *data) {\
        if (stack->size >= stack->allocated) {\
            stack_resize_##name(stack, stack->allocated << 1);\
        }\
        stack->array[stack->size++] = *data;\
    }\
    static inline data_t *stack_peek_##name(const stack_##name##_t *stack) {\
        if (stack->size) {\
        	return stack->array + (stack->size - 1);\
        }\
        else\
        	return NULL;\
    }

/*!
  @abstract Type of the stack.
  @param  name  Name of the stack [symbol]
 */
#define stack_t(name) stack_##name##_t
 
/*! @function
  @abstract     Initiate a stack.
  @param  name  Name of the stack [symbol]
  @return       Pointer to the stack [stack_t(name)*]
 */
#define stack_init(name) stack_init_##name()

/*! @function
  @abstract     Initiate a stack allocated statically.
  @param  name  Name of the stack [symbol]
  @return       Stack struct [stack_t(name)]
 */
#define stack_init_local(name) stack_init_local_##name()
 
/*! @function
  @abstract     Destroy a stack.
  @param  name  Name of the stack [symbol]
  @param  s     Pointer to the stack [stack_t(name)*]
 */
#define stack_destroy(name, s) stack_destroy_##name(s)

/*! @function
  @abstract     Deallocates stack array ad resets its internal values.
  @param  name  Name of the stack [symbol]
  @param  s     Pointer to the stack [stack(name)*]
 */
#define stack_free(name, s) stack_free_##name(s)
 
/*! @function
  @abstract     Reset a stack without deallocating memory.
  @param  name  Name of the stack [symbol]
  @param  s     Pointer to the stack [stack(name)*]
 */
#define stack_clear(name, s) stack_clear_##name(s)
 
/*! @function
  @abstract     Resize a stack.
  @param  name  Name of the stack [symbol]
  @param  h     Pointer to the stack [stack_t(name)*]
  @param  size     New size [size_t]
 */
#define stack_resize(name, s, size) stack_resize_##name(s, size)

/*! @function
  @abstract     Trims a stack, removing all extra allocations that were still in memory.
  @param  name  Name of the stack [symbol]
  @param  h     Pointer to the stack [stack_t(name)*]
  @param  size  New size [size_t]
 */
#define stack_trim(name, s, size) stack_resize_##name(s, s->size)
 
/*! @function
  @abstract     Insert a value int the stack.
  @param  name  Name of the stack [symbol]
  @param  s     Pointer to the stack [stack_t(name)*]
  @param  v     Pointer to the data to insert in the stack [data_t*]
 */
#define stack_push(name, s, v) stack_push_##name(s, v)
 
/*! @function
  @abstract     Pops an element from the top of the stack.
  @param  name  Name of the stack [symbol]
  @param  s     Pointer to the stack [stack_t(name)*]
  @return       Value of the removed element [data_t]
 */
#define stack_pop(name, s) stack_pop_##name(s)

/*! @function
  @abstract     Retrieve the top element value from the stack.
  @param  name  Name of the stack [symbol]
  @param  s     Pointer to the stack [stack_t(name)*]
  @return       Pointer to the top element of the stack [data_t*]
 */
#define stack_peek(name, s) stack_peek_##name(s)
 
/*! @function
  @abstract     Get the number of elements in the stack
  @param  name  Name of the stack [symbol]
  @param  s     Pointer to the stack [stack_t(name)*]
  @return       Number of elements in the stack [size_t]
 */
#define stack_size(name, s) ((s)->size)

/*! @function
  @abstract     Returns true if this stack contains no elements.
  @param  name  Name of the stack [symbol]
  @param  s     Pointer to the stack [stack_t(name)*]
  @return       1 if the stack is empty 0 otherwise [int]
 */
#define stack_empty(name, s) ((s)->size <= 0)

#endif /* STACK_H */
