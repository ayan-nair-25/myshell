#ifndef MYSH
#define MYSH
char * check_paths (char *);

int contains_slash(char *);

int check_if_builtin(char *);

void run_builtin_command(int, arraylist_t **);

void run_bare_name(arraylist_t **);

void print_exit_info(arraylist_t **, int);

int is_redirect(char *);

arraylist_t * redirect_inputs(arraylist_t **);

int is_pipe(char *);

arraylist_t * find_first_pipe(arraylist_t **);

arraylist_t * find_second_pipe(arraylist_t **);

int is_redir_pipe(arraylist_t **);

int run_execv(arraylist_t **);

int run_redirect(arraylist_t **);

int run_pipe(arraylist_t **, char *);

arraylist_t * detect_wildcard(arraylist_t *);

int is_conditional(arraylist_t **);

arraylist_t * omit_condition(arraylist_t **);

int run_conditional(arraylist_t **, char *);

int parse_command(char *, int);

arraylist_t * to_arraylist(char *, int);

char * read_input(int, int);

void run_batch_mode(char *, int);

void run_interactive_mode();
#endif
