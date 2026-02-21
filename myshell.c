// This is the code skeleton for Lab 2. If you do not need it, you may also build your code from scratch.

/*
Lab 2 – A Simple Shell

Requirements (Refer to the document for accuracy and details):
- internal commands: cd, clr, dir, environ, echo, help, pause, quit
- external commands: fork + exec
- redirection: <, >, >> (stdout redirection also for internal: dir/environ/echo/help)
- background: '&' at end => do not wait
- batch input: myshell batchfile (EOF => exit)
- env:
    shell=<fullpath>/myshell  (set in main)
    parent=<fullpath>/myshell (set in child before exec)
- prompt includes current directory

*/

#define _XOPEN_SOURCE 700 // This is for POSIX functions like realpath()

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#define MAX_LINE 1024 // Maximum length of input

//Other functions needed ...

// Reaping zombie processes from background execution
static void reap_zombies(void) {
    while (waitpid(-1, NULL, WNOHANG) > 0) { // Loop to clean up any child processes that haven't exited
    }
}

// Commands
static int process_line(char *line) {

    // Remove newline character from fgets
    char *newline = strchr(line, '\n');
    if (newline != NULL) {
        *newline = '\0'; // Instead of '\n' add the null terminator
    }

    // If empty line
    if (line[0] == '\0') {
        return 1; // Skip it and continue shell loop
    }

    // Extract first word as the command
    char *cmd = strtok(line, " \t"); // Set space and tab as delimiters
    if (cmd == NULL) { // If no command
        return 1; // Skip and continue with shell loop
    }

    // Vairables for I/O redirection and background execution
    
    char *outfile = NULL;      // Redirected output file
    char *infile = NULL;       // Redirected input file
    int append = 0;            // Flag for appending (0 --> overwrite file, 1 --> append to file)
    int background = 0;        // Flag for background execution (0 --> not a background command, 1 --> a background command) 
    char *args[64];            // Array of commands and arguments
    int nargs = 0;             // Counter of arguments
    args[nargs++] = cmd;       // Store first input, the command, in args[] and increment nargs
    char *tok;                 // Variable to parse the remaining words in the command line

    while ((tok = strtok(NULL, " \t")) != NULL) { // Check each word/token (delimiters are space and tab) until NULL is reached

        // Input redirection
        if (strcmp(tok, "<") == 0) {
            infile = strtok(NULL, " \t"); // Set the input file
        }

        // Output redirection (Overwrite)
        else if (strcmp(tok, ">") == 0) {
            outfile = strtok(NULL, " \t"); // Set the output file
            append = 0; // Overwrite file
        }

        // Output redirection (Append)
        else if (strcmp(tok, ">>") == 0) {
            outfile = strtok(NULL, " \t"); // Set the output file
            append = 1; // Append to file
        }

        // Background execution
        else if (strcmp(tok, "&") == 0) {
            background = 1; // Set the background execution flag
        }

        // Regular argument
        else {
            args[nargs++] = tok; // Store the argument in args[] and incrememnt nargs
        }
    }
    
    args[nargs] = NULL; // Indicate the end of the args array

    // Redirection for internal commands
    int saved_stdout = -1; // Indicates no redirection
    if (outfile != NULL) { // If an output file is set
        saved_stdout = dup(1);  // save original stdout

        FILE *f;
        // Append to the file
        if (append) {
            f = fopen(outfile, "a");
        }
        // Overwrite the file
        else {
            f = fopen(outfile, "w");
        }
        // Error opening file
        if (f == NULL) {
            perror("fopen"); // Return an error
            return 1;
        }

        dup2(fileno(f), 1);  // redirect stdout
        fclose(f);           // Close the file
    }

    // Internal commands

    // quit command
    if (strcmp(cmd, "quit") == 0) { // If the user types quit
        return 0; // End the shell loop
    }

    // environ command
    if (strcmp(cmd, "environ") == 0) { // If the user types environ

		// Access the environment variables using the global variable 'environ'
        extern char **environ;
        char **env = environ;

        // Print all the strings from the environ variable
        while (*env != NULL) {
            printf("%s\n", *env);
            env++;
        }

        // If the user used redirection
        if (saved_stdout != -1) {
            // Restore the original file descriptor
            dup2(saved_stdout, 1);
            // Close the backup
            close(saved_stdout);
        }
        return 1; // End of command, continue shell loop
    }

    // echo command
    if (strcmp(cmd, "echo") == 0) {
        // Print each argument
        for (int i = 1; args[i] != NULL; i++) {
            printf("%s", args[i]);
            if (args[i + 1] != NULL) { // Add a space between each argument
                printf(" ");
            }
        }

        printf("\n");

        // If the user used redirection
        if (saved_stdout != -1) {
            // Restore the original file descriptor
            dup2(saved_stdout, 1);
            // Close the backup
            close(saved_stdout);
        }

        return 1; // End of command, continue shell loop
    }

    // cd command
    if (strcmp(cmd, "cd") == 0) {

        char *dir = args[1]; // Get the directory argument

        // If no argument then print the current directory
        if (dir == NULL) {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            }
			// If there is an error getting the current directory, print an error message
            else {
                perror("getcwd");
            }
            return 1; // End of command, continue shell loop
        }

        // If changing directory fails, print an error message
        if (chdir(dir) != 0) {
            perror("cd");
            return 1;
        }

        // Update the PWD environment variable to reflect the new current directory
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            setenv("PWD", cwd, 1);
        }

        return 1; // End of command, continue shell loop
    }

    // clr command
    if (strcmp(cmd, "clr") == 0) {
        printf("\033[2J\033[H"); // Clear the screen and move the cursor to the top-left corner
        fflush(stdout); // Flush the output to ensure it is displayed immediately
        return 1; // End of command, continue shell loop
    }

    // help command
    if (strcmp(cmd, "help") == 0) {

        // If redirection is used
        if (outfile != NULL) {
			// Print the contents of the readme file to the redirected output
            FILE *f = fopen("readme", "r");
			// If there is an error opening the readme file, print an error message
            if (f == NULL) {
                perror("help");
            }
			// If the file is opened successfully, read and print its contents
            else {
                char buffer[1024]; // Buffer to hold each line of the readme file
				// Read each line from the readme file and print it to the redirected output
                while (fgets(buffer, sizeof(buffer), f)) {
                    printf("%s", buffer);
                }
                fclose(f); // Close the readme file after reading
            }
			// If the user used redirection
            if (saved_stdout != -1) {
				// Restore the original file descriptor
                dup2(saved_stdout, 1);
				// Close the backup
                close(saved_stdout);
            }

            return 1; // End of command, continue shell loop
        }

        // If no redirection then fork a child process to execute the more command to display the readme file
        pid_t pid = fork();

		// If the process id is negative, it means the fork failed
        if (pid < 0) {
            perror("fork"); // Print an error message
            return 1; // End of command, continue shell loop
        }
		// If process id is 0, this is the child process
        if (pid == 0) {
			// Set up the arguments for the more command to display the readme file
            char *args[] = {"more", "readme", NULL};
            execvp("more", args); // Execute the more command to display the readme file
            perror("execvp"); // If execvp returns, it means there was an error executing the command, so print an error message
            exit(1); // Exit the child process with an error code
        }
		// This is the parent process
        else {
            waitpid(pid, NULL, 0); // Wait for the child process
        }

        return 1; // End of command, continue shell loop
    }

    // pause command
    if (strcmp(cmd, "pause") == 0) {
		// Print a message prompting the user to press Enter to continue
        printf("Press Enter to continue...");
        fflush(stdout);  // make sure message is printed

		// Wait for the user to press Enter by reading characters until a newline is encountered
        while (getchar() != '\n') {
            ; // wait until newline
        }

        return 1; // End of command, continue shell loop
    }

    // dir command
    if (strcmp(cmd, "dir") == 0) {
		// Get the path argument for the dir command
        char *path = args[1];

        // If no path is provided, use current directory
        if (path == NULL) {
            path = "."; // Set path to current directory
        }
		
		// Open the specified directory
        DIR *dir = opendir(path);
		// If there was an error opening the directory
        if (dir == NULL) {
            perror("dir"); // Print an error
			// If the user used redirection
            if (saved_stdout != -1) {
				// Restore the original file descriptor
                dup2(saved_stdout, 1);
				// Close the backup
                close(saved_stdout);
            }
            return 1; // End of command, continue shell loop
        }
		// Read and print each entry in the directory
        struct dirent *entry;
		// Loop through the directory entries and print their names
        while ((entry = readdir(dir)) != NULL) {
            printf("%s\n", entry->d_name); // Print the name of the directory entry followed by a newline
        }
		// Close the directory stream after reading
        closedir(dir);
		 // If the user used redirection
        if (saved_stdout != -1) {
			// Restore the original file descriptor
            dup2(saved_stdout, 1);
			// Close the backup
            close(saved_stdout);
        }
        return 1; // End of command, continue shell loop
    }

    // Restore stdout (if not already restored)
    if (saved_stdout != -1) {
		// Restore the original file descriptor
        dup2(saved_stdout, 1);
		// Close the backup
        close(saved_stdout);
    }

    // ---------- EXTERNAL COMMAND ----------

	// Fork a child process to execute external commands
    pid_t pid = fork();

	// If the process id is negative, it means the fork failed
    if (pid < 0) {
        perror("fork"); // Print an error message
        return 1; // End of command, continue shell loop
    }
	// If process id is 0, this is the child process
    if (pid == 0) {

		// If an input file is set
        if (infile != NULL) {
			// Open the input file for reading
            FILE *f = fopen(infile, "r");
			// If there was an error opening the file
            if (f == NULL) {
                perror("fopen"); // Print an error
                exit(1); // Exit the child process with an error code
            }
			// Redirect stdin to the input file using dup2, which duplicates the file descriptor of the opened file onto the standard input (fd 0)
            dup2(fileno(f), 0);
            fclose(f); // Close the file after redirecting
        }

        // If an output file is set
        if (outfile != NULL) {
			// Open the output file for writing
            FILE *f;
			// If append flag is set
            if (append) {
				// Open the file in append mode
                f = fopen(outfile, "a");
            }
			// If append flag is not set
            else {
				// Open the file in overwrite mode
                f = fopen(outfile, "w");
            }
			// If there was an error opening the file
            if (f == NULL) {
                perror("fopen"); // Print an error
                exit(1); // Exit the child process with an error code
            }
			// Redirect stdout to the output file using dup2, which duplicates the file descriptor of the opened file onto the standard output (fd 1)
            dup2(fileno(f), 1);
            fclose(f); // Close the file after redirecting
        }

        // Set the "parent" environment variable to the value of "shell" if it exists
        if (getenv("shell") != NULL) {
            setenv("parent", getenv("shell"), 1);
        }
		// Execute the external command using execvp, which replaces the current process image with a new program specified by cmd and args
        execvp(cmd, args);
		// If execvp returns, it means there was an error executing the command, so print an error message
        perror("execvp");
        exit(1); // Exit the child process with an error code
    }
	// This is the parent process
    else {
		// If the background execution flag is not set
        if (!background) {
            waitpid(pid, NULL, 0); // Wait for the child process
        }
		// If the background execution flag is set
        else {
            printf("[background pid %d]\n", pid); // Print the background process ID to the user
        }
    }

    return 1; // End of command, continue shell loop
}

// Main function
int main(int argc, char *argv[]) {
    // Set environment variable "shell" to full path of myshell
    char fullpath[PATH_MAX];
	// Use realpath to resolve the full path of the myshell executable and store it in fullpath
    if (argc >= 1 && realpath(argv[0], fullpath) == NULL) {
        perror("realpath"); // If there was an error resolving the full path, print an error message
        exit(1); // Exit the program with an error code
    }
	// Set the "shell" environment variable to the full path of myshell using setenv
    if (setenv("shell", fullpath, 1) != 0) {
        perror("setenv"); // If there was an error setting the environment variable, print an error message
        exit(1); // Exit the program with an error code
    }
	// Initialize the input stream to standard input (stdin)
    FILE *in = stdin;
	// If a batch file is provided as a command-line argument, open it for reading and set the input stream to the file
    if (argc == 2) {
        in = fopen(argv[1], "r"); // Open the batch file for reading
		// If there was an error opening the batch file, print an error message and exit
        if (!in) {
			perror("fopen");
			return 1;
		}
	// If the batch file is opened successfully
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [batchfile]\n", argv[0]); // Print usage message if too many arguments are provided
        return 1; // Exit the program with an error code
    }

	// Buffer to hold the input line and current working directory
    char line[MAX_LINE];
	// Buffer to hold the current working directory, used for displaying the prompt
    char cwd[PATH_MAX];

	// Main shell loop
    while (1) {
		// Reap any zombie processes from background execution before displaying the prompt and reading the next command
        reap_zombies();

		// If the input stream is standard input
        if (in == stdin) {
            if (getcwd(cwd, sizeof(cwd)) == NULL) { // Check for errors in getting the current working directory
				perror("getcwd"); // If there was an error, print an error message
				break; // Exit the shell loop
			}
            printf(">myshell:%s$ ", cwd); // Display the prompt with the current working directory
            fflush(stdout); // Flush the output to ensure the prompt is displayed immediately
        }
		// Read a line of input from user or bactch file until EOF
        if (!fgets(line, sizeof(line), in))
			break; // If there was an error reading the line (e.g., EOF), break out of the shell loop

		// Process the input line and execute the command
        if (process_line(line) == 0) // If 0, then it is the quit command
			break; // Exit the shell loop
    }
	// If the input stream is not standard input
    if (in != stdin)
		fclose(in); // Close the batch file if it was opened

    return 0; // Exit the program with a success code
}

/*
========================
Guidance / Hints
Content: Chenyu NING
Email: chenyu.ning@ontariotechu.ca
Format: chatGPT (Be an honest person)

*This section contains function descriptions intended to guide you when implementing the corresponding features. 
*If you do not need this guidance, you may delete it directly. 
*Based on these descriptions, you should be able to complete this assignment independently.
========================

--------------------------------------------------
1.i cd
--------------------------------------------------
fgets(buffer, size, stdin)
    - reads an entire line from standard input stdin (until '\n' or EOF)
    - includes the '\n' character in buffer
    - returns buffer on success, NULL on failure
    - used in this shell to read a complete command entered by the user

strtok(char *str, const char *delim)
    - used to split a string using delimiters
    - *modifies the original string* (by inserting '\0')
    - this lab assumes all arguments are separated by spaces or tabs

strcmp(str1, str2)
    - compares two strings
    - returns 0 if they are exactly the same

getcwd(char *buf, size)
    - retrieves the current working directory
    - returns buf on success, NULL on failure

--------------------------------------------------
1.ii clr
--------------------------------------------------
fflush(stdout)
    - forcibly flushes the stdout buffer
    - ensures output is immediately displayed in the terminal

ANSI escape sequences
    - terminals interpret certain strings beginning with '\033' as control commands
    - commonly used for clearing the screen, moving the cursor, changing colors, etc.

Common examples:
    \033[2J        clear the screen
    \033[H         move cursor to the top-left corner
    \033[K         clear from cursor to end of line
    \033[2K        clear the entire line
    \033[row;colH  move cursor to the specified row and column
    \033[31m       red text
    \033[0m        reset styles

--------------------------------------------------
1.iii dir
--------------------------------------------------
strcat(dest, src)
    - appends src to the end of dest
    - requires dest to have sufficient space

DIR *opendir(const char *path)
    - opens a directory
    - returns DIR* on success, NULL on failure

struct dirent *readdir(DIR *dir)
    - reads an entry from the directory
    - returns a struct dirent* containing the filename and other information
    - returns NULL when no more entries are available

closedir(DIR *dir)
    - closes the directory stream and releases resources

--------------------------------------------------
1.iv environ
--------------------------------------------------
extern char **environ
    - a global variable provided by the C runtime
    - environ is a NULL-terminated array of strings
    - the environ command simply prints this array

--------------------------------------------------
1.vi help
--------------------------------------------------
fork()
    - creates a child process
    - both parent and child continue execution from the return point of fork
    - return values:
        * returns 0 in the child process
        * returns the child PID (>0) in the parent process
        * returns -1 on failure

Why must the shell use fork?
    - exec-family functions *replace the current process*
    - if the shell directly uses exec to run an external program, the shell itself will terminate
    - correct approach:
        shell (parent)
            └── fork
                  └── child -> exec external program

execvp(file, argv)
    - replaces the current process image with a new program
    - file: program name (searched in PATH)
    - argv: argument array, must be NULL-terminated
    - does not return on success, returns -1 on failure

wait(&status)
    - blocks until any child process terminates
    - status stores the child's exit status

waitpid(pid, &status, options)
    - waits for a specific child process
    - commonly used options:
        * 0        : blocking
        * WNOHANG  : non-blocking (used to reap zombie processes in background execution)

--------------------------------------------------
1.vii pause
--------------------------------------------------
getchar()
    - reads a single character from stdin
    - blocks if no input is available (used for pause)

--------------------------------------------------
1.ix shell environment
--------------------------------------------------
realpath(path, resolved_path)
    - resolves a relative path into an absolute path
    - used to set shell=<absolute_path>/myshell

setenv(name, value, overwrite)
    - sets an environment variable
    - overwrite=1 allows an existing value to be replaced

getenv(name)
    - retrieves the value of an environment variable
    - returns NULL if it does not exist

--------------------------------------------------
3. external command
--------------------------------------------------
fopen(filename, mode)
    - opens a file and returns a FILE*
    - common modes:
        "r"  read-only (error if file does not exist)
        "w"  write (create if not exists, truncate if exists)
        "a"  append (create if not exists)

FILE* vs file descriptor
    - FILE* is an abstraction provided by the C standard library
    - internally, it still interacts with the OS using file descriptors (int)
    - fileno(FILE*) retrieves the corresponding file descriptor

--------------------------------------------------
4. i/o-redirection
--------------------------------------------------
dup(fd)
    - duplicates a file descriptor
    - returns a new fd pointing to the same file

dup2(oldfd, newfd)
    - duplicates oldfd onto the specified descriptor number newfd
    - if newfd is already open, it is closed first

Core idea of redirection:
    - stdout is essentially file descriptor 1
    - by redirecting fd=1 to a file, printf will write to that file

Redirection strategy for internal commands:
    1. save the current stdout
    2. dup2 stdout to the file
    3. execute the internal command
    4. restore stdout

Redirection strategy for external commands:
    - modify stdin/stdout in the child process
    - exec the external program
    - this does not affect the parent shell

--------------------------------------------------
5. background execution
--------------------------------------------------

Zombie process
    - the child process has terminated
    - the parent process has not called wait
    - if the parent directly returns to the main loop instead of calling waitpid,
      it effectively implements background execution

waitpid(-1, NULL, WNOHANG)
    - does not block the process
    - reaps any terminated child process
    - returns immediately if no child process has terminated

*/
