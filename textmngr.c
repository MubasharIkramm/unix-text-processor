/* This is the only file you should update and submit. */

/* Fill in your Name and GNumber in the following two comment fields
 * Name:
 * GNumber:
 */

#include <sys/wait.h>
#include "textmngr.h"
#include "parse.h"
#include "util.h"

/* Constants */
#define DEBUG 1

/*
static const char *textproc_path[] = { "./", "/usr/bin/", NULL };
static const char *instructions[] = { "quit", "help", "list", "new", "open", "write", "close", "print", "active", "pause", "resume", "cancel", "exec", NULL};
*/

/* The entry of your text processor program */
int main() {
    char cmdline[MAXLINE];        /* Command line */
    char *cmd = NULL;

    /* Intial Prompt and Welcome */
    log_help();

    /* Shell looping here to accept user command and execute */
    while (1) {
        char *argv[MAXARGS];        /* Argument list */
        Instruction inst;           /* Instruction structure: check parse.h */

        /* Print prompt */
        log_prompt();

        /* Read a line */
        // note: fgets will keep the ending '\n'
	errno = 0;
        if (fgets(cmdline, MAXLINE, stdin) == NULL) {
            if (errno == EINTR) {
                continue;
            }
            exit(-1);
        }

        if (feof(stdin)) {  /* ctrl-d will exit text processor */
          exit(0);
        }

        /* Parse command line */
        if (strlen(cmdline)==1)   /* empty cmd line will be ignored */
          continue;     

        cmdline[strlen(cmdline) - 1] = '\0';        /* remove trailing '\n' */

        cmd = malloc(strlen(cmdline) + 1);
        snprintf(cmd, strlen(cmdline) + 1, "%s", cmdline);

        /* Bail if command is only whitespace */
        if(!is_whitespace(cmd)) {
            initialize_command(&inst, argv);    /* initialize arg lists and instruction */
            parse(cmd, &inst, argv);            /* call provided parse() */

            if (DEBUG) {  /* display parse result, redefine DEBUG to turn it off */
                debug_print_parse(cmd, &inst, argv, "main (after parse)");
	    }

            /* After parsing: your code to continue from here */
            /*================================================*/

	    
        }

	free(cmd);
	cmd = NULL;
        free_command(&inst, argv);
    }
    return 0;
}

