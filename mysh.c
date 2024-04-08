#include <errno.h>
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
			printf("Error, incorrect number of commands");
		}
		//printf("changing dir to %s\n", (*args)->data[1]);
		int res = chdir((*args)->data[1]);
		if (res < 0) {
			printf("Error: %s\n", strerror(errno));
		}
	}
	else if (res == 2) {
		char curr_dir[PATH_MAX + 1];
		printf("%s\n", getcwd(curr_dir, sizeof(curr_dir)));
		fflush(stdout);
	}
	else {
		// implement which		
		char * cmd_path = check_paths((*args)->data[1]);
		if (cmd_path != NULL) {
			printf("%s\n", cmd_path);
			fflush(stdout);
		}
	}
}

void run_bare_name(arraylist_t ** args) {
	char * cmd_path = check_paths((*args)->data[0]);
	if (DEBUG) {printf("executing %s\n", cmd_path);}
	printf("command found at %s\n", cmd_path);
	if (cmd_path != NULL) {
		char * tmp = (*args)->data[0];
		(*args)->data[0] = cmd_path;
		execv(cmd_path, (*args)->data);
		(*args)->data[0] = tmp;
	}
	else {
		printf("Command not found, exiting\n");
		exit(0);
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

int parse_command(char * command, int strlen) {
	if (DEBUG) {printf("currently parsing command %s with length %d\n", command, strlen);}
	if (strlen == 0) {
		return 0;	
	}

	arraylist_t * args = to_arraylist(command, strlen);
	int arr_len = al_length(args);

	if (DEBUG) {
		printf("printing arraylist\n");
		for (int i = 0; i < arr_len; i++) {
			printf("string: %s\n", (args)->data[i]);
		}
	}

	if (strcmp(args->data[0], "exit") == 0) {
		if (arr_len > 2) {
			print_exit_info(&args, arr_len);
			/*
			for (int i = 1; i < arr_len - 1; i++) {
				// only print spaces while we aren't at the last arg
				if (i != arr_len - 1) {
					printf("%s ", args->data[i]);
					fflush(stdout);
				}
				// otherwise just don't print a space
				else {
					printf("%s", args->data[i]);
					fflush(stdout);
				}
			}
			printf("\n");
			*/
		}
		printf("current process id %d\n", getpid());
		if (DEBUG) {printf("returning 1 from parse_command, exiting\n");}
		return 1;
	}

	//printf("checking command %s\n", command);
	if (check_if_builtin(args->data[0]) > 0) {
		//printf("we are here\n");
		int res = check_if_builtin(args->data[0]);
		run_builtin_command(res, &args);
		/*
		if (res == 1) {
			if (arr_len != 3) {
				printf("Error, incorrect number of commands");
			}
			int res = chdir(args->data[1]);
			if (res < 0) {
				printf("Error: %s\n", strerror(errno));
			}
		}
		else if (res == 2) {
			char curr_dir[PATH_MAX + 1];
			printf("%s\n", getcwd(curr_dir, sizeof(curr_dir)));
			fflush(stdout);
		}
		else {
			// implement which		
			char * cmd_path = check_paths(args->data[1]);
			if (cmd_path != NULL) {
				printf("%s\n", cmd_path);
				fflush(stdout);
			}
		}
		*/
	}
	else {

		int status;
		int child_pid;
		if ((child_pid = fork()) == 0) {
			printf("in child\n");
			if (DEBUG) {
				printf("in child\n");
				printf("parsing %s\n", args->data[0]);
				printf("contains a slash: %d\n", contains_slash(args->data[0]));
				printf("is builtin: %d\n", check_if_builtin(args->data[0]));
				printf("length of arg list: %d\n", arr_len);

				printf("printing arraylist\n");
				for (int i = 0; i < arr_len; i++) {
					printf("string: %s\n", (args)->data[i]);
				}

				printf("printing arraylist args\n");
				for (int i = 0; i < arr_len - 1; i++) {
					printf("string :%s\n", ((args->data) + 1)[i]);
				}
			}

			if (contains_slash(args->data[0])) {
				execv(args->data[0], args->data);
			}
			// add a check to see if the command is cd, pwd, etc.
			else {
				run_bare_name(&args);
				// the 3 dirs that we want to search
				/*
				char * cmd_path = check_paths(args->data[0]);
				if (DEBUG) {printf("executing %s\n", cmd_path);}
				printf("command found at %s\n", cmd_path);
				if (cmd_path != NULL) {
					char * tmp = args->data[0];
					args->data[0] = cmd_path;
					execv(cmd_path, args->data);
					args->data[0] = tmp;
				}
				else {
					printf("Command not found, exiting\n");
					exit(0);
				}
				free(cmd_path);
				*/
			}
			//exit(0);
		}

		wait(&status);
		
	}

	/*
	int status;
	int child_pid;
	if ((child_pid = fork()) == 0) {
		if (DEBUG) {
			printf("in child\n");
			printf("parsing %s\n", args->data[0]);
			printf("contains a slash: %d\n", contains_slash(args->data[0]));
			printf("is builtin: %d\n", check_if_builtin(args->data[0]));
			printf("length of arg list: %d\n", arr_len);

			printf("printing arraylist\n");
			for (int i = 0; i < arr_len; i++) {
				printf("string: %s\n", (args)->data[i]);
			}

			printf("printing arraylist args\n");
			for (int i = 0; i < arr_len - 1; i++) {
				printf("string :%s\n", ((args->data) + 1)[i]);
			}
		}

		if (contains_slash(args->data[0])) {
			execv(args->data[0], (args->data) + 1);
			exit(0);
		}
		// add a check to see if the command is cd, pwd, etc.
		else if (check_if_builtin(args->data[0]) > 0) {
			int res = check_if_builtin(args->data[0]);
			if (res == 1) {
				if (arr_len != 3) {
					printf("Error, incorrect number of commands");
				}
				int res = chdir(args->data[1]);
				if (res < 0) {
					printf("Error: %s\n", strerror(errno));
				}
			}
			else if (res == 2) {
				char curr_dir[PATH_MAX + 1];
				printf("%s\n", getcwd(curr_dir, sizeof(curr_dir)));
				fflush(stdout);
			}
			else {
				// implement which		
				char * cmd_path = check_paths(args->data[1]);
				if (cmd_path != NULL) {
					printf("%s\n", cmd_path);
					fflush(stdout);
				}
			}
		}
		else {
			// the 3 dirs that we want to search
			char * cmd_path = check_paths(args->data[0]);
			if (DEBUG) {printf("executing %s\n", cmd_path);}
			if (cmd_path != NULL) {
				char * tmp = args->data[0];
				args->data[0] = cmd_path;
				execv(cmd_path, args->data);
				args->data[0] = tmp;
			}
			free(cmd_path);
		}
		//exit(0);
	}
	*/
	al_destroy(args);
	return 0;
}

/*
void strip_newlines(arraylist_t ** al) {
	int arr_len = al_length(*al);
	for (int i = 0; i < arr_len; i++) {
		(*al)->data[i][strcspn((*al)->data[i], "\n")] = 0;	
	}
}
*/

// 1. given a full word, check if it has a *. if not then do normal stuff
// 2. use glob.h to get all matching strings with the pattern in the word
// 3. add all matching strings to arraylist
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
		if (curr_char == ' ' || curr_char == '\0') {
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
					// parse the command for batch mode	
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



void run_batch_mode() {
	return;
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
	char buf;
	int bytes;

	/*
	char * ex = "hello   there bro";
	arraylist_t * out = to_arraylist(ex, 17);
	*/

	int input_tty = isatty(STDIN_FILENO);
	int output_tty = isatty(STDOUT_FILENO);
	if (argc == 2) {
		if (DEBUG) {
			printf("Found file in cmd args. Running batch mode.\n");
		}
		run_batch_mode();
	}
	else if (input_tty == 0 && output_tty == 1) {
		if (DEBUG) {
			printf("Found piped input. Running batch mode.\n");
		}
		run_batch_mode();
	}
	else if (input_tty == 1 && output_tty == 1) {
		if (DEBUG) {
			printf("No input or pipe. Running interactive mode.\n");
		}
		run_interactive_mode();
	}

	return 0;
}
