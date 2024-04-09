Arul Elango : ake20
Ayan Nair: aan93


README
PROJECT 3:

In this project we established program called mysh.c that performs some of the basic functionality of a shell program for a UNIX system.
The capabilities of this shell program are enumerated below.
	1. Interactive and batch mode
		- Interactive mode is started when .mysh is called with no arguments
			-It will read commands from standard input until exited.
			-Will also print welcome and goodbye messages, along with a prompt
			 before reading in the next command.
		- Batch mode will read commands from the specified file(MAX 1 file), line by 
		  line until all lines within the file are exhausted.
	2. Redirection and Piping
		- In both interactive and batch modes, the shell will manage std_input and
		  std_output according to the known behaviors of '<', '>', and '|'.
		- Redirection is attended to first, then piping behaviors are dealt with.
	3. Executing commands
		- If the commands has a first arguments with a slash command will be
		  executed.
		- Commands cd, pwd, which, and exit are implemented by mysh itself.
			-Their functions match those of the same commands in the actual UNIX
		  	 shell.
		- Commands called that are not within the built-ins and do not have a slash
		  will be searched for in directories /usr/local/bin and /bin.
	4. Wildcards
		- If a command argument contains an asterisk *, the shell will replace that
		  asterisked argument with all files within the current directory that match
		  the filename according to known rules of wildcard expansion.
	5. Conditionals
		- If the first argument of a command the is 'then' or 'else', this will
		  affect the execution of the following command.
			-if it is 'then', only if the prior argument succeeded, the argument
			 following 'then' will be executed.
			-if it is 'else', only if the prior argument failed, will the
			 argument following 'else' be executed.
Our test plan is such to test each of the above 5 functionalities separately, and then lastly, all together to stress test the program. To do so the following steps can be taken.

We will perform the tests separately, in interactive mode, then we will perform the same ones in a batch file all together to show that the full functionality of the shell is present in both modes.

The Tests:

1. We will first use mysh to change directory to a directory we have provided with our submission.
	- command: 'cd testdir'
2. Then, use mysh to compile a c program that we will use for further testing.
	- command: 'gcc hitch.c -o hitch'
3. Next, use mysh to run a command with redirects and piping all with variable spacing.
   	- this will test our shell's ability to tokenize and parse complicated commands.
	- command: './hitch < ft.txt > bacterias.txt | ./hitch<ft.txt>bbuuss.txt'
4. 