#ifndef MYSH
#define MYSH
char * check_paths (char *);

int contains_slash(char *);

int parse_command(char *, int);

arraylist_t * to_arraylist(char *, int);

char * read_input(int, int);

void run_batch_mode();

void run_interactive_mode();
#endif
