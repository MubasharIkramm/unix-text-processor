/* This is the only file you should update and submit. */

/* Fill in your Name and GNumber in the following two comment fields
 * Name: Mubashar Ikram
 * GNumber: G01476721
 */

#include <sys/wait.h>
#include "textmngr.h"
#include "parse.h"
#include "util.h"
#include <signal.h>
#include <errno.h>

/* Constants */
#define DEBUG 1

/*
   static const char *textproc_path[] = { "./", "/usr/bin/", NULL };
   static const char *instructions[] = { "quit", "help", "list", "new", "open", "write", "close", "print", "active", "pause", "resume", "cancel", "exec", NULL};
   */

typedef struct{
	int id; //buffer identifier
	char *text; //buffer test content
	int state; //current state
	pid_t pid; //process id
	char *cmd; //full command line
	int output_fd; //file descriptor for process output pipe
}Buffer;

Buffer **buffers = NULL; //dynamic array of buffer pointers
int buffer_count = 0; //num of open buffers
int max_buffer_id = 0; //highest buf id assigned
int active_buffer_id = 0; //id of cur active buffer
char *current_cmdline = NULL; //current cmd line being processed

void handle_quit(void);
void handle_help(void);
void handle_list(void);
void handle_new(const Instruction *inst, char *argv[]);
void handle_active(const Instruction *inst);
Buffer *find_buffer(int buffer_id);
void handle_close(const Instruction *inst);
void remove_buffer(int buffer_id);
void free_buffer(Buffer *buffer);
void handle_print(const Instruction *inst);
void handle_open(const Instruction *inst);
void handle_write(const Instruction *inst);
void handle_exec(const Instruction *inst, char *argv[]);
void setup_signal_handlers(void);
void sigchld_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);
void handle_pause(const Instruction *inst);
void handle_resume(const Instruction *inst);
void handle_cancel(const Instruction *inst);

/* The entry of your text processor program */
int main() {
	char cmdline[MAXLINE];        /* Command line */
	char *cmd = NULL;

	/* Intial Prompt and Welcome */
	log_help();

	//Signal Handlers
	setup_signal_handlers();

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
		current_cmdline = cmd;

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

		if(strcmp(inst.instruct, "quit") == 0){
			handle_quit();
		}
		else if(strcmp(inst.instruct, "help") == 0){
			handle_help();
		}
		else if(strcmp(inst.instruct, "list") == 0){
			handle_list();
		}
		else if(strcmp(inst.instruct, "new") == 0){
			handle_new(&inst, argv);
		}
		else if(strcmp(inst.instruct, "active") == 0){
			handle_active(&inst);
		}
		else if(strcmp(inst.instruct, "close") == 0){
			handle_close(&inst);
		}
		else if(strcmp(inst.instruct, "print") == 0){
			handle_print(&inst);
		}
		else if(strcmp(inst.instruct, "open") == 0){
			handle_open(&inst);
		}
		else if(strcmp(inst.instruct, "write") == 0){
			handle_write(&inst);
		}
		else if(strcmp(inst.instruct, "exec") == 0) {
			handle_exec(&inst, argv);
		}
		else if(strcmp(inst.instruct, "pause") == 0){
			handle_pause(&inst);
		}
		else if(strcmp(inst.instruct, "resume") == 0){
			handle_resume(&inst);
		}
		else if(strcmp(inst.instruct, "cancel") == 0){
			handle_cancel(&inst);
		}

		free(cmd);
		cmd = NULL;
		free_command(&inst, argv);
	}
	return 0;
}

void handle_quit(void){
	log_quit();
	exit(0);
}

void handle_help(void){
	log_help();
}

Buffer *create_buffer(void){
	//Allocate memory for new buffer
	Buffer *buffer = malloc(sizeof(Buffer));
	if(!buffer) return NULL;

	//assign buffer next id
	buffer->id = ++max_buffer_id;

	//empty text
	buffer->text = strdup("");

	//starts in ready
	buffer->state = LOG_STATE_READY;
	buffer->pid = 0;
	buffer->cmd = NULL;
	buffer->output_fd = -1;

	return buffer;
}

int add_buffer(Buffer *buffer){
	//check if buffer exists
	if (!buffer) return -1;

	//increase array size by 1 and add buffer
	buffers = realloc(buffers, (buffer_count + 1) * sizeof(Buffer*));
	buffers[buffer_count] = buffer;
	buffer_count++;

	return 0;
}

void handle_list(void){
	//total numbers of buffers
	log_buf_count(buffer_count);

	//If no buffers
	if(buffer_count == 0){
		log_show_active(0);
		return;
	}

	//logging details for buffers
	for(int i = 0; i < buffer_count; i++){
		log_buf_details(buffers[i]->id, buffers[i]->state, buffers[i]->pid, buffers[i]->cmd);
	}

	//which is active
	log_show_active(active_buffer_id);

}	

void handle_new(const Instruction *inst, char *argv[]){
	//Create new buffer
	Buffer *new_buffer = create_buffer();
	if(!new_buffer) return;

	//add it to global array
	add_buffer(new_buffer);

	//Set as active buffer
	active_buffer_id = new_buffer->id;

	//log messages
	log_open(new_buffer->id);
	log_activate(new_buffer->id);

	//if arg provided execute
	if(argv[0] != NULL){
		handle_exec(inst, argv);
	}

}

void handle_active(const Instruction *inst){
    //get id
    int target_id = inst->id;

    //if no id
    if(target_id == 0){
        log_activate(active_buffer_id);
        return;
    }

    //Check if buffer exists
    Buffer *buffer = find_buffer(target_id);
    if(buffer == NULL){
        log_buf_id_error(target_id);
        return;
    }

    //Set as active buffer
    active_buffer_id = target_id;
    log_activate(active_buffer_id);

    // If the buffer has a working process, wait for it
    if(buffer->state == LOG_STATE_WORKING && buffer->pid > 0){
        int status;

        while(1){
            pid_t result = waitpid(buffer->pid, &status, WUNTRACED | WCONTINUED);

            if(result == -1){
                if(errno == EINTR){
                    continue;
                }
                break;
            }
		//child exited normally
            if(WIFEXITED(status)){
                log_cmd_state(buffer->pid, LOG_ACTIVE, buffer->cmd, LOG_CANCEL);

		//if exit code 0 load output
                if(WEXITSTATUS(status) == 0 && buffer->output_fd != -1){
                    free(buffer->text);
                    buffer->text = fd_to_text(buffer->output_fd);
                    close(buffer->output_fd);
                    buffer->output_fd = -1;
                }
		//resetting buffer state
                buffer->state = LOG_STATE_READY;
                buffer->pid = 0;
                free(buffer->cmd);
                buffer->cmd = NULL;
                break;
            }

            else if(WIFSIGNALED(status)){
                log_cmd_state(buffer->pid, LOG_ACTIVE, buffer->cmd, LOG_CANCEL_SIG);

                if(buffer->output_fd != -1){
                    close(buffer->output_fd);
                    buffer->output_fd = -1;
                }

                buffer->state = LOG_STATE_READY;
                buffer->pid = 0;
                free(buffer->cmd);
                buffer->cmd = NULL;
                break;
            }
            else if(WIFSTOPPED(status)){
                log_cmd_state(buffer->pid, LOG_ACTIVE, buffer->cmd, LOG_PAUSE);
                buffer->state = LOG_STATE_PAUSED;
                break;
            }
            else if(WIFCONTINUED(status)){
                log_cmd_state(buffer->pid, LOG_ACTIVE, buffer->cmd, LOG_RESUME);
                buffer->state = LOG_STATE_WORKING;
            }
        }
    }
}

void handle_close(const Instruction *inst){
	int target_id = inst->id;

	//if no id
	if(target_id == 0){
		target_id = active_buffer_id;
	}

	//check buffer exists
	Buffer *buffer = find_buffer(target_id);
	if(buffer == NULL){
		log_buf_id_error(target_id);
		return;
	}

	//check if buffer is ready(else cant close)
	if(buffer->state != LOG_STATE_READY){
		log_close_error(target_id);
		return;
	}

	//removing buffer
	remove_buffer(target_id);

	//log close
	log_close(target_id);

	//if we closed active buffer set new active
	if(target_id == active_buffer_id){
		if(buffer_count == 0){
			active_buffer_id = 0;
		}
		else{
			//find largest
			int largest_id = 0;
			for(int i = 0; i < buffer_count; i++){
				if(buffers[i]->id > largest_id){
					largest_id = buffers[i]->id;
				}
			}
			active_buffer_id = largest_id;
			log_activate(active_buffer_id);
		}
	}
}

void free_buffer(Buffer *buffer){
	if(!buffer) return;

	if(buffer->text){
		free(buffer->text);
	}

	if(buffer->cmd){
		free(buffer->cmd);
	}

	free(buffer);

}

Buffer *find_buffer(int buffer_id){
    for(int i = 0; i < buffer_count; i++){
        if(buffers[i]->id == buffer_id){
            return buffers[i];
        }
    }
    return NULL; // not found
}

void remove_buffer(int buffer_id){
	//Find buffer in array
	int found_index = -1;

	for(int i = 0; i < buffer_count; i++){
		if(buffers[i]->id == buffer_id){
			found_index = i;
			break;
		}
	}

	if(found_index == -1) return; //buffer not found

	//free buffer
	free_buffer(buffers[found_index]);

	//shift buffers left
	for(int i = found_index; i < buffer_count - 1; i++){
		buffers[i] = buffers[i + 1];
	}

	buffer_count--;
	buffers = realloc(buffers, buffer_count * sizeof(Buffer*));
}


void handle_print(const Instruction *inst){
	int target_id = inst->id;

	//If no id
	if(target_id == 0){
		target_id = active_buffer_id;
	}

	//Check buffer exists
	Buffer *buffer = find_buffer(target_id);
	if(buffer == NULL){
		log_buf_id_error(target_id);
		return;
	}

	//Log buffer contents
	log_print(target_id, buffer->text);
}

void handle_open(const Instruction *inst){
	//Check filename
	if(inst->file == NULL){
		//no name
		return;
	}

	//Open file for reading
	int fd = open(inst->file, O_RDONLY);
	if(fd == -1){
		log_file_error(LOG_FILE_OPEN_READ, inst->file);
		return;
	}

	//New buffer
	Buffer *new_buffer = create_buffer();
	if(!new_buffer){
		close(fd);
		return;
	}

	//Read file contents into buffer
	free(new_buffer->text); // free empty string
	new_buffer->text = fd_to_text(fd);
	close(fd);

	//close buffers and set active
	add_buffer(new_buffer);
	active_buffer_id = new_buffer->id;

	//log messages
	log_open(new_buffer->id);
	log_activate(new_buffer->id);
	log_read(new_buffer->id, inst->file);

}

void handle_write(const Instruction *inst){
	int target_id = inst->id;

	//no id
	if(target_id == 0){
		target_id = active_buffer_id;
	}

	//Check buffer exists
	Buffer *buffer = find_buffer(target_id);
	if(buffer == NULL){
		log_buf_id_error(target_id);
		return;
	}

	//check for filename
	if(inst->file == NULL){
		//error
		return;
	}

	//Open file to write
	int fd = open(inst->file, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if(fd == -1){
		log_file_error(LOG_FILE_OPEN_WRITE, inst->file);
		return;
	}

	//Write buffer info to file
	text_to_fd(buffer->text, fd);
	close(fd);

	log_write(target_id, inst->file);
} 

void handle_exec(const Instruction *inst, char *argv[]){
	int target_id = inst->id;

	if(target_id == 0){
		target_id = active_buffer_id;
		if(target_id == 0){
			log_buf_id_error(0);
			return;
		}
	}

	Buffer *buffer = find_buffer(target_id);
	if(buffer == NULL){
		log_buf_id_error(target_id);
		return;
	}

	if(buffer->state != LOG_STATE_READY){
		log_cmd_state_conflict(target_id, buffer->state);
		return;
	}

	int stdin_pipe[2];
	int stdout_pipe[2];

	if(pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1){
		return;
	}

	pid_t pid = fork();
	if(pid == -1){
		close(stdin_pipe[0]); close(stdin_pipe[1]);
		close(stdout_pipe[0]); close(stdout_pipe[1]);
		return;
	}

	if(pid == 0){
		//child process
		setpgid(0, 0);

		//reset signal handlers to default
		signal(SIGINT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		signal(SIGCHLD, SIG_DFL);

		close(stdin_pipe[1]);
		close(stdout_pipe[0]);

		dup2(stdin_pipe[0], STDIN_FILENO);
		close(stdin_pipe[0]);

		dup2(stdout_pipe[1], STDOUT_FILENO);
		close(stdout_pipe[1]);

		char local_path[256];
		snprintf(local_path, sizeof(local_path), "./%s", argv[0]);

		char full_path[256];
		snprintf(full_path, sizeof(full_path), "/usr/bin/%s", argv[0]);

		if (execv(local_path, argv) == -1 && execv(full_path, argv) == -1) {
			exit(1);
		}
	}
	else{
		//parent process
		int is_active = (target_id == active_buffer_id);
		buffer->state = LOG_STATE_WORKING;
		buffer->pid = pid;
		buffer->cmd = strdup(current_cmdline);
		log_start(target_id, pid, is_active ? LOG_ACTIVE : LOG_BACKGROUND, current_cmdline);

		close(stdin_pipe[0]);
		close(stdout_pipe[1]);

		text_to_fd(buffer->text, stdin_pipe[1]);
		close(stdin_pipe[1]);

		if(is_active){
			//for active process: wait directly and handle state changes
			int status;
			while(1){
				pid_t result = waitpid(pid, &status, WUNTRACED | WCONTINUED);
				if(result == -1){
					if(errno == EINTR){
						continue;  //interrupted by signal, retry
					}
					break;
				}

				if(WIFEXITED(status)){
					//process exited logging it
					log_cmd_state(pid, LOG_ACTIVE, buffer->cmd, LOG_CANCEL);

					//only replace text if exit status was 0
					if(WEXITSTATUS(status) == 0){
						free(buffer->text);
						buffer->text = fd_to_text(stdout_pipe[0]);
					}
					close(stdout_pipe[0]);

					//reset buffer state
					buffer->state = LOG_STATE_READY;
					buffer->pid = 0;
					free(buffer->cmd);
					buffer->cmd = NULL;
					break;
				}
				else if(WIFSIGNALED(status)){
					//process terminated by signal
					log_cmd_state(pid, LOG_ACTIVE, buffer->cmd, LOG_CANCEL_SIG);
					close(stdout_pipe[0]);

					//reset buffer state
					buffer->state = LOG_STATE_READY;
					buffer->pid = 0;
					free(buffer->cmd);
					buffer->cmd = NULL;
					break;
				}
				else if(WIFSTOPPED(status)){
					//process was paused
					log_cmd_state(pid, LOG_ACTIVE, buffer->cmd, LOG_PAUSE);
					buffer->state = LOG_STATE_PAUSED;
					buffer->output_fd = stdout_pipe[0];
					break;  //exit the wait loop
				}
				else if(WIFCONTINUED(status)){
					//process resumed
					log_cmd_state(pid, LOG_ACTIVE, buffer->cmd, LOG_RESUME);
					buffer->state = LOG_STATE_WORKING;
					//continue waiting
				}
			}
		} else {
			//background process, saveing output pipe for signal handler to read later
			buffer->output_fd = stdout_pipe[0];
		}
	}
}
void setup_signal_handlers(void){
	struct sigaction sa;

	//setting up sigchld hadnler
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &sa, NULL);

	//setting up sigint handler
	sa.sa_handler = sigint_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGINT, &sa, NULL);

	//setting up sigtstp handler
	sa.sa_handler = sigtstp_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &sa, NULL);

}

void sigchld_handler(int sig){
	int status;
	pid_t pid;

	//reaping the complete child processes
	while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0){
		Buffer *buffer = NULL;
		for(int i = 0; i < buffer_count; i++){
			if(buffers[i]->pid == pid){
				buffer = buffers[i];
				break;
			}
		}

		if(!buffer) continue;

		if(WIFEXITED(status)){
			log_cmd_state(pid, (buffer->id == active_buffer_id) ? LOG_ACTIVE : LOG_BACKGROUND,buffer->cmd, LOG_CANCEL); //process exited normally

			if(buffer->output_fd != -1){
				if(WEXITSTATUS(status) == 0){
					free(buffer->text);
					buffer->text = fd_to_text(buffer->output_fd);
				}
				close(buffer->output_fd);
				buffer->output_fd = -1;
			}
			//reset buffer state
			buffer->state = LOG_STATE_READY;
			buffer->pid = 0;
			free(buffer->cmd);
			buffer->cmd = NULL;
		}else if (WIFSIGNALED(status)){
			log_cmd_state(pid, (buffer->id == active_buffer_id) ? LOG_ACTIVE : LOG_BACKGROUND,buffer->cmd, LOG_CANCEL_SIG); //process terminated by signal
			
			if(buffer->output_fd != -1){
				close(buffer->output_fd);
				buffer->output_fd = -1;
			}

			buffer->state = LOG_STATE_READY;
			buffer->pid = 0;
			free(buffer->cmd);
			buffer->cmd = NULL;
		}else if(WIFSTOPPED(status)){
			//process was stopped
			log_cmd_state(pid, (buffer->id == active_buffer_id) ? LOG_ACTIVE : LOG_BACKGROUND, buffer->cmd, LOG_PAUSE);
			buffer->state = LOG_STATE_PAUSED;
		}else if(WIFCONTINUED(status)){
			//process continued
			log_cmd_state(pid, (buffer->id == active_buffer_id) ? LOG_ACTIVE : LOG_BACKGROUND, buffer->cmd, LOG_RESUME);
			buffer->state = LOG_STATE_WORKING;
		}
	}
}


void sigint_handler(int sig){
	log_ctrl_c(); //Log Ctrl C was pressed

	//give to active buffer if running process
	Buffer *buffer = find_buffer(active_buffer_id);
	if(active_buffer_id != 0){
		if (buffer && buffer->state == LOG_STATE_WORKING){
			kill(buffer->pid, SIGINT); //give sigint to active process
		}
	}

}

void sigtstp_handler(int sig){
	log_ctrl_z(); //Log Ctrl Z was pressed

	//give to active buffer if running process
	if(active_buffer_id != 0){
		Buffer *buffer = find_buffer(active_buffer_id);
		if(buffer && (buffer->state == LOG_STATE_WORKING)){
			kill(buffer->pid, SIGTSTP); //give sigstp to active process
		}
	}

}

void handle_cancel(const Instruction *inst){
	int target_id = inst->id;

	if(target_id == 0){
		target_id = active_buffer_id;
	}

	Buffer *buffer = find_buffer(target_id);
	if(buffer == NULL){
		log_buf_id_error(target_id);
		return;
	}

	if(buffer->state != LOG_STATE_WORKING && buffer->state != LOG_STATE_PAUSED){
		log_cmd_state_conflict(target_id, buffer->state);
		return;
	}

	if(buffer->state == LOG_STATE_PAUSED){
		kill(buffer->pid, SIGCONT);
	}

	//SIGINT to cancel process
	kill(buffer->pid, SIGINT);
	log_cmd_signal(LOG_CMD_CANCEL, target_id);
}

void handle_pause(const Instruction *inst){
	int target_id = inst->id;

	if(target_id == 0){
		target_id = active_buffer_id;
	}

	Buffer *buffer = find_buffer(target_id);
	if(buffer == NULL){
		log_buf_id_error(target_id);
		return;
	}

	if(buffer->state != LOG_STATE_WORKING && buffer->state != LOG_STATE_PAUSED){
		log_cmd_state_conflict(target_id, buffer->state);
		return;
	}

	//SIGSTP to pause process
	kill(buffer->pid, SIGTSTP);
	log_cmd_signal(LOG_CMD_PAUSE, target_id);
}

void handle_resume(const Instruction *inst){
	int target_id = inst->id;

	if(target_id == 0){
		target_id = active_buffer_id;
	}

	Buffer *buffer = find_buffer(target_id);
	if(buffer == NULL){
		log_buf_id_error(target_id);
		return;
	}

	if(buffer->state != LOG_STATE_WORKING && buffer->state != LOG_STATE_PAUSED){
		log_cmd_state_conflict(target_id, buffer->state);
		return;
	}

	// Send SIGCONT to continue process
	kill(buffer->pid, SIGCONT);
	log_cmd_signal(LOG_CMD_RESUME, target_id);

	if(target_id == active_buffer_id){
		int status;
		while(1){
			pid_t result = waitpid(buffer->pid, &status, WUNTRACED | WCONTINUED);
			if(result == -1){
				if(errno == EINTR){
					continue;
				}
				break;
			}

			if(WIFEXITED(status)){
				log_cmd_state(buffer->pid, LOG_ACTIVE, buffer->cmd, LOG_CANCEL);
				if(WEXITSTATUS(status) == 0 && buffer->output_fd != -1){
					free(buffer->text);
					buffer->text = fd_to_text(buffer->output_fd);
					close(buffer->output_fd);
					buffer->output_fd = -1;
				}
				buffer->state = LOG_STATE_READY;
				buffer->pid = 0;
				free(buffer->cmd);
				buffer->cmd = NULL;
				break;
			}
			else if(WIFSIGNALED(status)){
				log_cmd_state(buffer->pid, LOG_ACTIVE, buffer->cmd, LOG_CANCEL_SIG);
				buffer->state = LOG_STATE_READY;
				buffer->pid = 0;
				free(buffer->cmd);
				buffer->cmd = NULL;
				break;
			}
			else if(WIFSTOPPED(status)){
				log_cmd_state(buffer->pid, LOG_ACTIVE, buffer->cmd, LOG_PAUSE);
				buffer->state = LOG_STATE_PAUSED;
				break;
			}
			else if(WIFCONTINUED(status)){
				//process resumed, log it and wait
				log_cmd_state(buffer->pid, LOG_ACTIVE, buffer->cmd, LOG_RESUME);
				buffer->state = LOG_STATE_WORKING;
				//continue the loop to wait for next state change
			}
		}
	}
}








