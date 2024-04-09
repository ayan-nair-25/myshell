#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ArrayList.h"
// our arraylist struct is defined in our header and hence we don't define it here

// create our init and destroy functions
void al_init(arraylist_t * L, unsigned size) {;
	// initalize all the fields of our arraylist here
	L->data = malloc(size * sizeof(char *));
	L->length = 0;
	L->capacity = size;
}

// deallocate all of our memory here
void al_destroy(arraylist_t * L) {
	for (int i = 0; i < L->length; i++) {
		if (L->data[i] != NULL) {
			free(L->data[i]);
		}
	}
	if (L->data != NULL) {
		free(L->data);	
	}
}	

unsigned al_length(arraylist_t * L) {
	return L->length;
}

// write to the end of the array, unless the array is already full
// in this case we make the array bigger

/*
 * NOTE ON REALLOC
 *
 * void * realloc(void *, size_t); <- function prototype
 *
 * int * q = realloc(p, new_size); (in this example, either p and q point to the same object, or p now points to dead space and q holds the actual object
 *
 * 	if realloc has enough space, then q and p will point to the same object. else p will point to dead space (we assume that any pointer has been moved in implementation however)
 *
 * 	hence we need to update all pointers to an object if we use realloc on it, as we are unsure of where these old pointers will point to
 *
 * Another example
 *
 * p = realloc(p, size); (this is bad, as realloc can fail and then our pointer would automatically become a dead pointer)
 *
 *	 however the original object to which p pointed to is now lost and cannot be free
 */

void al_push(arraylist_t * L, char * data) {
	if (L->length == L->capacity) {
		// we use realloc to do this, realloc will make a new object and copy all the old data into the new object
		L->capacity = 2*L->capacity;
		// L->data = realloc(L->data, L->capacity * sizeof(int)); do not do this because of the above problems outlined
		
		char ** temp = realloc(L->data, L->capacity * sizeof(char *));
		if (!temp) {
			// this means that realloc failed, and so we can indicate failure to our caller in a general codebase
			// 	realloc fails when we run out of memory (not a crazy concern for our super good computers)
			// for our own purposes we can do whatever we want tho
			// deal with this in some way
			fprintf(stderr, "Out of memory!\n");
			// exit will terminate the program and returns some exit status to the failure
			exit(EXIT_FAILURE);
		}
		// now we can reassign the data of our arraylist after safely reallocating
		L->data = temp;
	}
	L->data[L->length] = data;
	L->length = L->length + 1;
}

// pop needs to return if the pop was successful + the element that we popped itself
int al_pop(arraylist_t * L, char ** dest) {
	// we return 1 on success, write popped item to dest, and return 0 on failure when the list is empty	
	if (L->length == 0) return 0;
	
	L->length--;
	// note length starts at 1, so we decrement and then return the element at the end, which corresponds to the last
	// element of the list
	// then we write it to our destination pointer
	*dest = L->data[L->length];
	return 1;
}
