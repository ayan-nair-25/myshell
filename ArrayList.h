// in our header we define our struct and everything else that we need
typedef struct{
	// our pointer for our integers
	char ** data;
	// keep track of our list size
	// make it unsigned as it will be positive
	unsigned length;
	// need to know the actual capacity of the array
	// ** note - unsigned is just shorthand for unsigned int **
	unsigned capacity;
} arraylist_t;

// create our init and destroy functions
// also define prototypes for all of the other functions that we want to define
void al_init(arraylist_t *, unsigned);
void al_destroy(arraylist_t *);

unsigned al_length(arraylist_t *);

// pop needs to return if the pop was successful + the element that we popped itself
void al_push(arraylist_t *, char *);
int al_pop(arraylist_t *, char **);
