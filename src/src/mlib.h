#ifndef MLIB_H
#define MLIB_H

typedef struct {
	void* ptr;
	void (*free_func) (void* ptr); // custom free function
} ptr_w_t; // pointer wrapper type

/*! @function
  @abstract     		Scope a function call in a separate mlib stack record. All pointers registered while the function is running will be freed right after the functions return.
  @param FUNCTION_CALL  Function call [symbol]
  @param ... 			Additional parameters for the passed function [__VA_ARGS__]
 */
#define SCOPE(FUNCTION_CALL)\
	push();\
	FUNCTION_CALL;\
	pop();

/**
 * Registers a pointer in the map associated with a scope.
 * The system will free the registered pointer upon closing of the specified scope.
 **/
void *reg_ptr(void* ptr);

/**
 * Registers a custom pointer in the map associated with a scope.
 * The system will free the registered pointer upon closing of the specified scope.
 * In this case the free function is passed in input by the user
 **/
void *reg_ptr_fn(void* ptr, void (*free_func)(void*));

/**
 * Registers directly a pointer wrapper in the current scope.
 * The system will free the registered pointer (using the defined function if not null) upon closing of the specified scope.
 **/
void *reg_ptr_w(ptr_w_t ptr_w);

/**
 * Push a new scope on the stack, all the variable registered after this function call
 * will be available until the pop function is called.
 **/
void push();

/**
 * Pop last scope on the stack, all the variables registered after the last push call
 * will be freed.
 **/
void pop();

/**
 * Pop last scope on the stack but transfer ownership of the passed pointer (it wont be freed)
 **/
void *pop_trn(void* ptr);

/**
 * Pop last scope but transfer ownership of the passed pointer (it wont be freed)
 * the return value of this function is the pointer wrapper that can be registered in the
 * parent scope to preserve the passed free function.
 **/
ptr_w_t pop_trn_w(void* ptr);

#endif
