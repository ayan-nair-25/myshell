#include <errno.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <linux/limits.h>

#include "ArrayList.h"
#include "mysh.h"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define BUF_SIZE 32

#ifndef DEBUG
#define DEBUG 0
#endif

int exit_status;

// batch mode is 0, interactive mode is 1
int contains_slash(char * command) {
	if (strchr(command, '/') != NULL) {
		return 1;	
	}
	else {
		return 0;	
	}
}

char * check_paths (char * command) {
	char * dir1 = "/usr/local/bin/";
	int pathlen1 = strlen(dir1);

	char * dir2 = "/usr/bin/";
	int pathlen2 = strlen(dir2);

	char * dir3 = "/bin/";
	int pathlen3 = strlen(dir3);

	int cmd_length = strlen(command);
	char * full_path = malloc(pathlen1 + cmd_length);

	memcpy(full_path, dir1, pathlen1);
	memcpy(full_path + pathlen1, command, cmd_length);
	if (DEBUG) {printf("searching for %s\n", full_path);}
	if (access(full_path, F_OK) == 0) {
		return full_path;
	}
	else {
		if (DEBUG) {printf("%s not found\n", full_path);}
		free(full_path);
		full_path = malloc(pathlen2 + cmd_length);

		memcpy(full_path, dir2, pathlen2);
		memcpy(full_path + pathlen2, command, cmd_length);

		if (DEBUG) {printf("searching for %s\n", full_path);}
		if (access(full_path, F_OK) == 0) {
			return full_path;
		}
		else {
			if (DEBUG) {printf("%s not found\n", full_path);}
			free(full_path);
			full_path = malloc(pathlen3 + cmd_length);

			memcpy(full_path, dir3, pathlen3);
			memcpy(full_path + pathlen3, command, cmd_length);
			if (DEBUG) {printf("searching for %s\n", full_path);}
			if (access(full_path, F_OK) == 0) {
				return full_path;
			}
		}
	
	}
	if (DEBUG) {printf("%s not found", full_path);}
	free(full_path);
	return NULL;
}

// return int > 0 if the command is equal to cd, pwd or which. we do not check for exit because we handle it separately
// 1 - cd
// 2 - pwd
// 3 - which
int check_if_builtin(char * command) {
	if (strcmp(command, "cd") == 0) {
		return 1;
	}
	else if (strcmp(command, "pwd") == 0) {
		return 2;	
	}
	else if (strcmp(command, "which") == 0) {
		return 3;	
	}
	else {
		return 0;	
	}
}

void run_builtin_command(int res, arraylist_t ** args) {
	int arr_len = al_length(*args);
	if (res == 1) {
		if (arr_len != 3) {
			printf("Error, incorrect number of commands\n");
			exit_status = 1;
		}
		//printf("changing dir to %s\n", (*args)->data[1]);
		int res = chdir((*args)->data[1]);
		if (res < 0) {
			printf("Error: %s\n", strerror(errno));
			exit_status = 1;
		}
	}
	else if (res == 2) {
		char curr_dir[PATH_MAX + 1];
		printf("%s\n", getcwd(curr_dir, sizeof(curr_dir)));
		fflush(stdout);
	}
	else {
		// implement which		
		if (check_if_builtin((*args)->data[1]) > 0) {
			exit_status = 1;
		}
		else if (arr_len != 3) {
			exit_status = 1;
		}
		char * cmd_path = check_paths((*args)->data[1]);
		if (cmd_path != NULL) {
			printf("%s\n", cmd_path);
			fflush(stdout);
		}
		else {
			exit_status = 1;
		}
	}
}

void run_bare_name(arraylist_t ** args) {
	char * cmd_path = check_paths((*args)->data[0]);
	if (DEBUG) {printf("executing %s\n", cmd_path);}
	if (cmd_path != NULL) {
		char * tmp = (*args)->data[0];
		(*args)->data[0] = cmd_path;
		execv(cmd_path, (*args)->data);
		(*args)->data[0] = tmp;
	}
	else {
		printf("Command not found, exiting\n");
		exit_status = 1;
		free(cmd_path);
		exit(exit_status);
	}
	free(cmd_path);
}

void print_exit_info(arraylist_t ** args, int arr_len) {
	for (int i = 1; i < arr_len - 1; i++) {
		if (i != arr_len - 1) {
			printf("%s ", (*args)->data[i]);
			fflush(stdout);
		}
		else {
			printf("%s", (*args)->data[i]);
			fflush(stdout);
		}
	}
	printf("\n");
}

int is_redirect(char * command) {
	if (strchr(command, '>') != NULL || strchr(command, '<') != NULL) {
		return 1;	
	}
	return 0;
}

arraylist_t * redirect_inputs(arraylist_t ** args) {
	arraylist_t * new_args = (arraylist_t *) malloc(sizeof(arraylist_t));
	al_init(new_args, 1);

	int orig_len = al_length(*args);
	int redir_mode = -1;
	int prev_redir = 0;

	for (int i = 0; i < orig_len - 1; i++) {
		if (prev_redir == 1) {
			if (redir_mode == 0) {
				int fd = open((*args)->data[i], O_RDONLY);
				if (fd > 0) {
					dup2(fd, STDIN_FILENO);
					close(fd);
				}
			}
			else if (redir_mode == 1) {
				int fd = open((*args)->data[i], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP);
				if (fd > 0) {
					dup2(fd, STDOUT_FILENO);
					close(fd);
				}
				else if (fd == -1) {
					printf("Unable to open file for reading.\n");
					exit_status = 1;
				}
			}
			prev_redir = 0;
			redir_mode = -1;
		}
		else if (strcmp((*args)->data[i], "redir_in") == 0) {
			prev_redir = 1;	
			redir_mode = 0;
		}
		else if (strcmp((*args)->data[i], "redir_out") == 0) {
			prev_redir = 1;
			redir_mode = 1;
		}
		else {
			int orig_length = strlen((*args)->data[i]);
			char * arg = malloc(orig_length + 1);
			memcpy(arg, (*args)->data[i], orig_length);
			arg[orig_length] = '\0';
			al_push(new_args, arg);
		}
	}
	al_push(new_args, (char *) NULL);
	return new_args;
}

int is_pipe(char * command) {
	if (strchr(command, '|') != NULL) {
		return 1;
	}
	else {
		return 0;	
	}
}

arraylist_t * find_first_pipe(arraylist_t ** args) {
	int arr_len = al_length(*args);
	arraylist_t * arglist = (arraylist_t *) malloc(sizeof(arraylist_t *));
	al_init(arglist, 1);

	for (int i = 0; i < arr_len; i++) {
		if (strcmp((*args)->data[i], "pipe") == 0) {
			al_push(arglist, (char *) NULL);
			return arglist;
		}
		al_push(arglist, (*args)->data[i]);
	}
}

arraylist_t * find_second_pipe(arraylist_t ** args) {
	int arr_len = al_length(*args);
	arraylist_t * arglist = (arraylist_t *) malloc(sizeof(arraylist_t *));
	al_init(arglist, 1);

	int append = 0;
	for (int i = 0; i < arr_len - 1; i++) {
		if (append == 1) {
			al_push(arglist, (*args)->data[i]);
		}
		if (strcmp((*args)->data[i], "pipe") == 0) {
			append = 1;	
		}
	}
	al_push(arglist, (char *) NULL);
	return arglist;
}

int is_redir_pipe(arraylist_t ** args) {
	int arr_len = al_length(*args);

	for (int i = 0; i < arr_len - 1; i++) {
		if (strcmp((*args)->data[i], "redir_in") == 0 || strcmp((*args)->data[i], "redir_out") == 0) {
			return 1;	
		}
	}
	
	return 0;
}

int run_execv(arraylist_t ** args) {
	// check for exit
	// then check for builtin
	int arr_len = al_length(*args);
	if (strcmp((*args)->data[0], "exit") == 0) {
		if (arr_len > 2) {
			print_exit_info(args, arr_len);
		}
		return 1;
	}

	//printf("checking command %s\n", command);
	if (check_if_builtin((*args)->data[0]) > 0) {
		//printf("we are here\n");
		int res = check_if_builtin((*args)->data[0]);
		run_builtin_command(res, args);
	}
	else {
		int status;
		int child_pid;
		if ((child_pid = fork()) == 0) {
			// if command is redirection command
			if (contains_slash((*args)->data[0])) {
				int res = execv((*args)->data[0], (*args)->data);
				if (res == -1) {
					exit_status = 1;	
				}
			}
			// add a check to see if the command is cd, pwd, etc.
			else {
				run_bare_name(args);
				// the 3 dirs that we want to search
			}
			exit(exit_status);
		}

		waitpid(child_pid, &status, 0);
		if( !WIFEXITED(status) ) {
			exit_status = 1;	
		}
		exit_status = WEXITSTATUS(status);
	}

	return 0;

}

int run_redirect(arraylist_t ** args) {
	int orig_stdout = dup(STDOUT_FILENO);
	int orig_stdin = dup(STDIN_FILENO);
	arraylist_t * new_args = redirect_inputs(args);
	int res = run_execv(&new_args);
	dup2(orig_stdout, 1);
	dup2(orig_stdin, 0);
	al_destroy(new_args);
	
	return res;
}

int run_pipe(arraylist_t ** args, char * command) {
	
	arraylist_t * arglist_1 = find_first_pipe(args);
	arraylist_t * arglist_2 = find_second_pipe(args);

	int p[2];
	int res = pipe(p);
	if (res == -1) {
		printf("Error on pipe\n");
		exit_status = 1;
	}
	int retval;

	int status1;
	int status2;

	//int child_pid_1 = fork();
	int child_pid_1;
	int child_pid_2;
	if ((child_pid_1 = fork()) == 0) {

		int redir = is_redir_pipe(&arglist_2);
		int ret;

		// dup2 to set stdout of first process to write end of pipe
		// if command is redirection command
		dup2(p[0], STDIN_FILENO);
		close(p[1]);
		if (redir) {
			ret = run_redirect(&arglist_2);
		}
		else {
			ret = run_execv(&arglist_2);
		}
		exit(ret);
	}
	if ((child_pid_2 = fork()) == 0) {
		int ret;
		int redir = is_redir_pipe(&arglist_1);

		dup2(p[1], STDOUT_FILENO);
		close(p[0]);
		if (redir) {
			ret = run_redirect(&arglist_1);
		}
		else {
			ret = run_execv(&arglist_1);
		}
		exit(ret);
	}
	close(p[0]);
	close(p[1]);

	waitpid(child_pid_1, &status1, 0);
	if( !WIFEXITED(status1) ) {
		exit_status = 1;	
		printf("exit status: %d\n", exit_status);
	}
	retval = WEXITSTATUS(status1);
	wait(&status2);

	return retval;
}


arraylist_t * to_arraylist(char * str, int strlen) {
	// initialize our arraylist
	arraylist_t * cmd_args = (arraylist_t *) malloc(sizeof(cmd_args));
	if (DEBUG) {printf("initializing\n");}
	// init w size 5
	al_init(cmd_args, 5);
	if (DEBUG) {printf("initialized\n");}
	// keep track of where we are in the string
	int stridx = 0;
	// also keep track of where our cmd starts for ptr arithmetic
	int strstart = 0;
	// while we have not hit the null terminator
	while (stridx <= strlen) {
		// store current char
		char curr_char = str[stridx];
		if (DEBUG) {printf("checking chr\n");}
		// if we are at a space or a null term and it's not a newline
		if (curr_char == ' ' || curr_char == '\0'|| curr_char == '<'|| curr_char == '>'|| curr_char == '|') {
			if (DEBUG) {printf("adding to list\n");}
			if (DEBUG) {printf("strstart: %d, stridx: %d\n", strstart, stridx);}
			// check that the start and the idx aren't equal, if this is the case then we
			// have likely found multiple spaces
			if (strstart != stridx) {
				// create word
				char * word = malloc(stridx - strstart + 1);
				// copy from main string into our word
				memcpy(word, str + strstart, stridx - strstart);
				// turn it into a string
				word[stridx - strstart] = '\0';
				if (DEBUG) {printf("copied %s from src string\n", word);}
				// push into arraylist
				al_push(cmd_args, word);
				if (DEBUG) {
					int length = al_length(cmd_args);
					printf("pushed %s\n", (cmd_args)->data[length - 1]);
				}
			}
			if(curr_char == '<'){
				char * str1 = malloc(9);
				strcpy(str1, "redir_in");
				str1[8] = '\0';
				al_push(cmd_args, str1);
			}
			if(curr_char == '>'){
				char * str1 = malloc(10);
				strcpy(str1, "redir_out");
				str1[9] = '\0';
				al_push(cmd_args, str1);
			}
			if(curr_char == '|'){
				char * str1 = malloc(5);
				strcpy(str1, "pipe");
				str1[4] = '\0';
				al_push(cmd_args, str1);
			}
			
			
			// get the new start of our string
			strstart = stridx + 1;
		}
		stridx++;
	}

	// print out arraylist for debug
	if (DEBUG) {
		printf("printing arraylist\n");
		int arr_len = al_length(cmd_args);
		for (int i = 0; i < arr_len; i++) {
			printf("string: %s\n", (cmd_args)->data[i]);
		}
		printf("\n");
	}
	// push the NULL to complete arg list for execv
	al_push(cmd_args, NULL);
	return cmd_args;
}

arraylist_t * detect_wildcard(arraylist_t * tokens){
	char newpath[PATH_MAX+1];
	arraylist_t * expanded_tokens = (arraylist_t *) malloc(sizeof(expanded_tokens));
	al_init(expanded_tokens, 5);
	glob_t globlist;
	int match = 0;
	for(int i = 0; i < al_length(tokens) - 1; i++){
		for(int j = 0; j < strlen(tokens->data[i]); j++){
			if(tokens->data[i][j] == '*'){
				match = 1;
				glob(tokens->data[i], GLOB_NOCHECK, NULL, &globlist);
				int k = 0;
				while (globlist.gl_pathv[k]){
					al_push(expanded_tokens, globlist.gl_pathv[k]);
          				k++;
				}

			}

		}
		if(match == 0){
			al_push(expanded_tokens, tokens->data[i]);
		}
		else{
			match = 0;
		}

	}
	al_push(expanded_tokens, (char *) NULL);
	return expanded_tokens;
}

int is_conditional(arraylist_t ** args) {
	if (strcmp((*args)->data[0], "then") == 0 ) {
		return 1;	
	}
	else if (strcmp((*args)->data[0], "else") == 0) {
		return 2;	
	}
	else {
		return 0;
	}
}

arraylist_t * omit_condition(arraylist_t ** args) {
	arraylist_t * new_args = (arraylist_t *) malloc(sizeof(arraylist_t *));
	al_init(new_args, 1);
	int arr_len = al_length(*args);

	for (int i = 1; i < arr_len - 1; i++) {
		al_push(new_args, (*args)->data[i]);
	}
	al_push(new_args, (char *) NULL);
	return new_args;
}

int run_conditional(arraylist_t ** args, char * command) {
	int condition_type = is_conditional(args);
	arraylist_t * new_args = omit_condition(args);

	int outval;
	int redir = is_redirect(command);
	int pipe = is_pipe(command);

	if (condition_type == 1 && exit_status == 0) {
		if (pipe) {
			outval = run_pipe(&new_args, command);
		}
		else if (redir) {
			outval = run_redirect(&new_args);
		}
		else {
			outval = run_execv(&new_args);
		}
	}
	else if (condition_type == 0 && exit_status == 1) {
		if (pipe) {
			outval = run_pipe(&new_args, command);
		}
		else if (redir) {
			outval = run_redirect(&new_args);
		}
		else {
			outval = run_execv(&new_args);
		}
	}

	return outval;

}

int parse_command(char * command, int strlen) {
	if (DEBUG) {printf("currently parsing command %s with length %d\n", command, strlen);}
	if (strlen == 0) {
		return 0;	
	}

	int redir = is_redirect(command);
	int pipe = is_pipe(command);
	arraylist_t * args = detect_wildcard(to_arraylist(command, strlen));
	int arr_len = al_length(args);
	int conditional = is_conditional(&args);
	/*
	for (int i = 0; i < arr_len - 1; i++) {
		printf("%s\n", (args)->data[i])	;
	}
	*/

	int outval;

	if (conditional > 0) {
		outval = run_conditional(&args, command);
	}
	else if (pipe) {
		outval = run_pipe(&args, command);
	}
	else if (redir) {
		outval = run_redirect(&args);
	}
	else {
		outval = run_execv(&args);
	}
	return outval;
}

// 1. given a full word, check if it has a *. if not then do normal stuff
// 2. use glob.h to get all matching strings with the pattern in the word
// 3. add all matching strings to arraylist
char * read_input(int fd, int mode) {
	
	int buflength = BUF_SIZE;
	int bytes = 1;

	char * buf = malloc(buflength);
	int pos = 0;

	// (bytes = read(fd, buf + pos, buflength - pos)) > 0
	while(bytes > 0) {
		/*if (mode == 1) {
			printf("mysh> ");
			fflush(stdout);
		}
		*/
		bytes = read(fd, buf + pos, buflength - pos);
		int linestart = 0;
		// assume that line begins at the start of the buffer
		int bufend = pos + bytes;
		if (DEBUG) { printf("read %d bytes; pos=%d\n", bytes, pos); }
		// mark down how many bytes are currently in the buffer
		while (pos < bufend) {
			if (DEBUG) {printf("linestart %d, pos %d, char %c\n", linestart, pos, buf[pos]);}
			// if char is a newline then we have found a line, where the line starts at linestart and ends before pos
			if(buf[pos] == '\n') {
				// indicate that the string ends at position pos so that we can print it
				buf[pos] = '\0';
				// pos - linestart gives the number of bytes
				// buf + linestart gives the beginning of the start of the line

				if (mode == 1) {
					exit_status = 0;
					/*
					int res = parse_command(buf + linestart, pos - linestart);
					if (DEBUG) {printf("strlen: %d\n", pos - linestart);}
					if (res == 1) {
						return (buf + linestart);
					}
					*/
					return (buf + linestart);
				}
				else {
					exit_status = 0;
					// parse the command for batch mode	
					parse_command(buf + linestart, strlen(buf + linestart));
				}
				//printf("%d chars: |%s|\n", pos - linestart, buf + linestart);

				// now we set the start of the next line to be the end of the line + 1
				linestart = pos + 1;
			}
			pos++;
		}

		if (linestart == pos) {
			// do nothing	
			pos = 0;
		}
		// if we have a partial line at the end of the buffer
		// we copy this partial line to the beginning of the buffer, so then we can call read again and get the rest of the bytes
		else if (linestart > 0) {
			int partial_line_length = pos - linestart;
			// move the buffer to the start
			memmove(buf, buf + linestart, partial_line_length);
			pos = partial_line_length;
			if (DEBUG) {printf("move %d bytes to the buffer start\n", partial_line_length);}
		}
		// in this case we have the entire buffer containing one partial line, hence our buffer needs more space
		// to counteract this we douvle the size of our buffer
		else {
			buflength *= 2;
			buf = realloc(buf, buflength);
		}
		// should check whether, after this there is still a partial line the buffer that didn't end w \n
		if (pos > 0) {
			if (DEBUG) {printf("%d bytes left over\n", pos);}
		}
	}
}

void run_batch_mode(char * path, int batch_type) {
	if (batch_type == 0) {
		read_input(STDIN_FILENO, 0);
	}
	else {
		int fd = open(path, O_RDONLY);
		dup2(fd, STDIN_FILENO);
		read_input(STDIN_FILENO, 0);
	}
}

// have a loop that runs and prints myshell
// and then run the input loop as part of this main loop
void run_interactive_mode() {
	printf("Welcome to my shell!\n");
	int res = 0;
	while (res == 0) {
		printf("mysh> ");
		fflush(stdout);
		char * command = read_input(STDIN_FILENO, 1);
		res = parse_command(command, strlen(command));
		if (DEBUG) {printf("returned value after parsing: %d\n", res);}
	}
	printf("mysh: exiting\n");
}

// IF argc == 2:
	// RUN BATCH MODE
// ELSE IF isatty(stdin_fileno) IS 0 AND isatty(stdout_fileno) IS 1:
	// RUN_BATCH_MODE
// ELSE
	// RUN_INTERACTIVE_MODE


int main(int argc, char ** argv) {
	int input_tty = isatty(STDIN_FILENO);
	int output_tty = isatty(STDOUT_FILENO);
	if (argc == 2) {
		if (DEBUG) {
			printf("Found file in cmd args. Running batch mode.\n");
		}
		int batch_type = 1;
		run_batch_mode(argv[1], batch_type);
	}
	else if (input_tty == 0 && output_tty == 1) {
		if (DEBUG) {
			printf("Found piped input. Running batch mode.\n");
		}
		int batch_type = 0;
		run_batch_mode(STDIN_FILENO, batch_type);
	}
	else if (input_tty == 1 && output_tty == 1) {
		if (DEBUG) {
			printf("No input or pipe. Running interactive mode.\n");
		}
		run_interactive_mode();
	}

	return 0;
}
