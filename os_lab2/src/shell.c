#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/command.h"
#include "../include/builtin.h"

// ======================= requirement 2.3 =======================
/**
 * @brief 
 * Redirect command's stdin and stdout to the specified file descriptor
 * If you want to implement ( < , > ), use "in_file" and "out_file" included the cmd_node structure
 * If you want to implement ( | ), use "in" and "out" included the cmd_node structure.
 *
 * @param p cmd_node structure
 * 
 */
void redirection(struct cmd_node *p) {
    // Input redirection: if in_file exists, redirect standard input to in_file
    if (p->in_file) {
        int in_fd = open(p->in_file, O_RDONLY); // Open in_file in read-only mode
        if (in_fd == -1) {
            perror("open input file");
            exit(EXIT_FAILURE); // Exit if opening fails
        }
        if (dup2(in_fd, STDIN_FILENO) == -1) { // Redirect in_fd to standard input
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(in_fd); // Close the temporary file descriptor
    }

    // Output redirection: if out_file exists, redirect standard output to out_file
    if (p->out_file) {
        int out_fd = open(p->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open or create out_file
        if (out_fd == -1) {
            perror("open output file");
            exit(EXIT_FAILURE);
        }
        if (dup2(out_fd, STDOUT_FILENO) == -1) { // Redirect out_fd to standard output
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(out_fd); // Close the temporary file descriptor
    }
    /*
    if(p->out != 1){
		dup2(p->out,STDOUT_FILENO);
	}
	if(p->in != 0){
		dup2(p->in,STDIN_FILENO);
	}
    */
}
// ===============================================================

// ======================= requirement 2.2 =======================
/**
 * @brief 
 * Execute external command
 * The external command is mainly divided into the following two steps:
 * 1. Call "fork()" to create child process
 * 2. Call "execvp()" to execute the corresponding executable file
 * @param p cmd_node structure
 * @return int 
 * Return execution status
 */
int spawn_proc(struct cmd_node *p)
{
	pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
		
		redirection(p);

        // using execvp to execute external command
        if (execvp(p->args[0], p->args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // wait
        int status;
        waitpid(pid, &status, 0); // Wait for the child process and get its status
        if (WIFEXITED(status)) { // Child process exited normally
            return 1; 
        } else {
            return -1;
        }
    }
}
// ===============================================================


// ======================= requirement 2.4 =======================
/**
 * @brief 
 * Use "pipe()" to create a communication bridge between processes
 * Call "spawn_proc()" in order according to the number of cmd_node
 * @param cmd Command structure  
 * @return int
 * Return execution status 
 */
int fork_cmd_node(struct cmd *cmd) {
    struct cmd_node *current = cmd->head;
    int pipe_fd[2];               // File descriptors for creating a pipe; pipe_fd[0]: read, pipe_fd[1]: write
    int in_fd = STDIN_FILENO;     // Initial input set to standard input
    int in = dup(STDIN_FILENO), out = dup(STDOUT_FILENO);   // Save the original standard input and output

    while (current != NULL) {
        // If there is a next command node, create a new pipe
        if (current->next != NULL) {
            if (pipe(pipe_fd) == -1) {
                perror("pipe");
                return -1;
            }
            // Redirect the current command's output to the write end of the pipe
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[1]);  // Close the write end of the pipe 
            //note that (close(pipe_fd[1]) is not the same as close(STDOUT_FILENO))
        }
        else{  // If it's the last command, redirect standard output to 'out' (screen) to display results
            dup2(out, 1);
            close(out);
        }
        
        if (in_fd != STDIN_FILENO) {
            dup2(in_fd, STDIN_FILENO); // Redirect input
            close(in_fd);
        }

        int status = searchBuiltInCommand(current);
        if (status != -1){  //built-in
            int built_in_in = dup(STDIN_FILENO), built_in_out = dup(STDOUT_FILENO);
            if( built_in_in == -1 | built_in_out == -1)
                perror("dup");
            redirection(current);
            status = execBuiltInCommand(status,current);

            // recover shell stdin and stdout
            if (current->in_file){
                dup2(built_in_in, 0);
            }
            if (current->out_file){
                dup2(built_in_out, 1);
            }
            close(built_in_in);
            close(built_in_out);
        }
        else{
            if (spawn_proc(current) == -1) {
                perror("spawn_proc");
                return -1;
            }
        }

        in_fd = pipe_fd[0];

        current = current->next;
    }

    dup2(in, 0);  // Restore standard input
    close(in);

    return 1;
}
// ===============================================================


void shell()
{
	while (1) {
		printf(">>> $ ");
		char *buffer = read_line();
		if (buffer == NULL)
			continue;

		struct cmd *cmd = split_line(buffer);
		
		int status = -1;
		// only a single command
		struct cmd_node *temp = cmd->head;
		
		if(temp->next == NULL){
			status = searchBuiltInCommand(temp);
			if (status != -1){  //built-in
				int in = dup(STDIN_FILENO), out = dup(STDOUT_FILENO);
				if( in == -1 | out == -1)
					perror("dup");
				redirection(temp);
				status = execBuiltInCommand(status,temp);

				// recover shell stdin and stdout
				if (temp->in_file){
					dup2(in, 0);
				}
				if (temp->out_file){
					dup2(out, 1);
				}
				close(in);
				close(out);
			}
			else{
				//external command
				status = spawn_proc(cmd->head);
			}
		}
		// There are multiple commands ( | )
		else{
			status = fork_cmd_node(cmd);
		}
		// free space
		while (cmd->head) {
			
			struct cmd_node *temp = cmd->head;
      		cmd->head = cmd->head->next;
			free(temp->args);
   	    	free(temp);
   		}
		free(cmd);
		free(buffer);
		
		if (status == 0)
			break;
	}
}
