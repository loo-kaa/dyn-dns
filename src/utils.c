#include "utils.h"

#include <sysexits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

#include <sys/stat.h>

#define STARTING_SIZE 20
#define MAX_SIZE 20 * 1000 * 1000

/**
 * reads a file until EOF or the declared MAX_SIZE
 **/
char* read_file(FILE* file_ptr) {
	size_t size = STARTING_SIZE, read = 0;
	char* buffer = c_malloc(size);

	int current;
	while((current = getc(file_ptr)) != EOF) {
		buffer[read++] = current;

		if(read >= size) {
			long new_size = size *= 2; // always double heuristic

			if(new_size <= MAX_SIZE) {
				buffer = c_realloc(buffer, new_size);
			}
			else {
				break;
			}
		}
	}
	
	// add termination char at the end
	buffer = c_realloc(buffer, read + 1);
	buffer[read] = 0;
	return buffer;
}

/**
 * reads characters from a input stream until \n or EOF or MAX_SIZE is reached
 **/
char* read_line(FILE* file_ptr) {
	size_t size = STARTING_SIZE;
	char* buffer = c_malloc(size);

	long first_file_pos = ftell(file_ptr),
		last_buffer_pos;

	char* fgets_result = fgets(buffer, size, file_ptr);

	/**
	 * while
	 * 	last_buffer_pos (current file position - file position before read) is greater than 0
	 * AND
	 *  fgets_result != NULL
	 * AND
	 *  buffer at position last_buffer_pos - 1 is not a new line
	 * AND
	 * 	the eof flag is not set
	 **/
	// TODO optimize and check logic to use fgets result
	while((last_buffer_pos = (ftell(file_ptr) - first_file_pos)) > 0 && fgets_result != NULL && buffer[last_buffer_pos - 1] != '\n' && !feof(file_ptr)) {
		long new_size = size *= 2; // always double heuristic

		if(new_size <= MAX_SIZE) {
			buffer = c_realloc(buffer, new_size);
		}
		else {
			break;
		}

		fgets_result = fgets(buffer + last_buffer_pos, new_size - last_buffer_pos, file_ptr);
		size = new_size;
	}

	if(last_buffer_pos > 0) {
		// realloc the buffer to be of the right size
		if(buffer[last_buffer_pos - 1] == '\n') {
			// in case that we read something AND the last char is a newline we remove the last char and place a string terminator
			buffer = c_realloc(buffer, last_buffer_pos);
			buffer[last_buffer_pos - 1] = 0;
		}
		else {
			// if the last char is not a new line we keep a byte for the string terminator 
			buffer = c_realloc(buffer, last_buffer_pos + 1);
		}
		
		return buffer;
	}
	else {
		// in the case last_buffer_pos is 0 we were not able to read so we free the allocated block and return NULL
		free(buffer);
		return NULL;
	}
}

#define MAX_ELEMENTS 20 * 1000

/**
 * Reads simple key values delimited by the equal sign
 * Returns a NULL terminated array
 * If MAX_SIZE is reached no other value will be read
 **/
struct property* read_property_file(FILE* file_ptr) {
	size_t size = STARTING_SIZE,
		read = 0;
	struct property* buffer = c_malloc(size * sizeof(struct property));
	char *line, *temp, *value;
	bool reading = true;

	while(reading && (line = read_line(file_ptr)) != NULL) {
		// remove stuff after the '#' sign
		temp = strtok(line, "#");
		// remove starting spaces
		temp = strtok(temp, " ");

		if(line != NULL && strlen(temp) > 0) {
			// splits the key from the value at the = sign
			temp = strtok(temp, "=");
			value = strtok(NULL, "=");

			if(temp != NULL && value != NULL) {
				buffer[read].key = c_malloc(strlen(temp) + 1);
				strcpy(buffer[read].key, temp);
				
				buffer[read].value = c_malloc(strlen(value) + 1);
				strcpy(buffer[read].value, value);

				if(++read >= size) {
					size_t new_size = size * 2; // always double heuristic

					if(new_size <= MAX_ELEMENTS) {
						buffer = c_realloc(buffer, new_size * sizeof(struct property));
					}
					else {
						// if we are over MAX_ELEMENTS stop reading 
						reading = false;
					}
				}
			}
		}

		free(line);
	}

	buffer = c_realloc(buffer, (read + 1) * sizeof(struct property));

	buffer[read] = (struct property){
		.key = NULL,
		.value = NULL
	};

	return buffer;
}

void free_properties(struct property *properties) {
	size_t i = 0;
	while(properties[i].key != NULL) {
		free(properties[i].key);
		free(properties[i].value);

		++i;
	}
	free(properties);	
}

char* get_property_value(struct property *properties, char *key) {
	size_t i = 0;
	while(properties[i].key != NULL) {
		if(strcmp(properties[i].key, key) == 0) {
			return properties[i].value;
		}

		++i;
	}

	return NULL;
}

#define FORMAT_CHAR '%'
#define STRING_FORMAT_CHAR 's'

/**
 * Expected as parameter a format that only containes "%s" and a list of strings only
 **/
char* format_string(char* format, ...) {
	va_list args;
	va_start(args, format);

	size_t size = strlen(format), i = 0;
	char current = '\0';
	bool matching_string = false;
	char* buffer = NULL;

	// can definitely be improoved
	while((current = format[i++]) != '\0') {
		if(!matching_string) {
			if(current == FORMAT_CHAR) {
				matching_string = true;
			}
		}
		else {
			if(current == STRING_FORMAT_CHAR) {
				size += strlen(va_arg(args, char*));
				size -= 2; // here we remove the size of the "%s" from the string
			}
			// will match only one char after the STRING_FORMAT_CHAR but its ok since we only need to match "%s"
			matching_string = false;
		}
	}

	buffer = c_malloc(size + 1);

	va_end(args);
	// repeat args loop
	va_start(args, format);
	
	vsprintf(buffer, format, args);

	va_end(args);

	return buffer;
}

char* strstr_block(char* str, const char* word, char block) {
	char current;
	size_t count = 0,
		i = 0,
		wordLength = strlen(word);

	while((current = str[i++]) != '\0' && current != block) {
		// we search for the word in the str 
        if(str[i] == word[count]) {
        	++count;

			// when we find the complete word we return 
			if(count == wordLength)
            	return str + (i - wordLength);
		}
		else {
			count = 0;
		}
	}

	return NULL;
}
