// Christian Rivera
// Shell Application
//
// Tested on Linux systems
// Supports Unix commands

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEBUG 0
#define INPUT_LIMIT 500
#define WORD_LIMIT 20
#define WORD_SIZE 100


//--------------------------------------------------------------------------------
// Handler for SIGINT
static void handler1(int sig) {
	char string[] = "SIGINT handled\nshell> ";
	write(1, string, sizeof(string));
}


//--------------------------------------------------------------------------------
// Handler for SIGTSTP
static void handler2(int sig) {
	char string[] = "SIGTSTP handled\nshell> ";
	write(1, string, sizeof(string));
}


//--------------------------------------------------------------------------------
// Prompts for input and parses input.  Returns number of words parsed.
static int getInput(char **args, char *inFile, char *outFile, int *append) {
	char buffer[INPUT_LIMIT];
	int i;

	// Collect user input
	printf("shell> ");
	fgets(buffer, INPUT_LIMIT, stdin);
	char *token = strtok(buffer, " \t\r\n");

	// Parse input
	i = 0;
	while (token) {
		if (DEBUG) {
			printf("Parsing: \'%s\'\n", token);
		}

		// Check for redirection
		if (strcmp(token, ">") == 0) {
			token = strtok(NULL, " \t\r\n");
			if (token != NULL) {
				strcpy(outFile, token);
			}
		}
		else if (strcmp(token, ">>") == 0) {
			token = strtok(NULL, " \t\r\n");
			if (token != NULL) {
				strcpy(outFile, token);
				*append = 1;
			}
		}
		else if (strcmp(token, "<") == 0) {
			token = strtok(NULL, " \t\r\n");
			if (token != NULL) {
				strcpy(inFile, token);
			}
		}
		else {
			// Allocate and copy arguement
			args[i] = (char*)malloc(sizeof(char) * WORD_SIZE);
			strcpy(args[i], token);
			i++;
		}
		if (token) {
			token = strtok(NULL, " \t\r\n");
		}
	}

	int j;
	for (j = i; j < WORD_LIMIT; j++) {
		args[j] = NULL;
	}

	return i;
}


//--------------------------------------------------------------------------------
// Fork and execute command.
static void doFork(char **args, char *inFile, char *outFile, int append) {
	int pid = fork();
	if (pid < 0) {
		fprintf(stderr, "* * * Fork Error\n");
		// exit(0);
	}
	else if (pid == 0) {
		// Child Process

		// Set redirection
		if (inFile[0] != '\0') {
			int fd = open(inFile, O_RDONLY);
			dup2(fd, 0);
		}
		if (outFile[0] != '\0') {
			int fd;
			if (append) {
				fd = open(outFile, O_WRONLY | O_CREAT | O_APPEND, S_IWUSR | S_IROTH);
			}
			else {
				fd = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IROTH);
			}
			dup2(fd, 1);
		}

		// Execute
		if (execvp(args[0], args) < 0) {
			printf("* * * Exec Error\n");
			exit(1);
		}
	}
	else {
		// Parent process
		printf("PID: %d\n", pid);
		
		int status;
		int result = waitpid(-1, &status, 0);

		if (result < 1) {
			printf("* * * There was an error reaping the child.\n");
		}
		else if (WIFEXITED(status)) {
			printf("EXIT: %d\n", WEXITSTATUS(status));
		}

	}
}

//--------------------------------------------------------------------------------
int main() {
	signal(SIGINT, handler1);
	signal(SIGTSTP, handler2);

	// Begin command loop
	int quit = 0;
	while (!quit) {
		char *args[WORD_LIMIT];
		char inFile[WORD_SIZE], outFile[WORD_SIZE];
		inFile[0] = '\0';
		outFile[0] = '\0';
		int i, append = 0;

		int wordCount = getInput(args, inFile, outFile, &append);
		if (wordCount < 1) {
			continue;
		}

		if (DEBUG) {
			printf("Words parsed: %d\n", wordCount);
			for (i = 0; i < wordCount; i++) {
				printf("%s\n", args[i]);
			}
		}

		// Check for quit
		if (strcmp(args[0], "quit") == 0) {
			quit = 1;
		}
		else {
			doFork(args, inFile, outFile, append); // Fork
		}

		// Free memory
		for (i = 0; i < wordCount; i++) {
			free(args[i]);
		}

	} // End of command loop

	return 0;
}



