#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include<fcntl.h>
#include <sys/wait.h> 
#include <sys/types.h>
#include <signal.h>
#define MAX_JOBS 10

typedef struct job{
	pid_t pid;					// process id
	pid_t pgid;					// process group id
	char name[24];				// job name
}job;

// job info
job job_table[MAX_JOBS];		// array of jobs
int no_of_jobs = 0;				
int job_status[MAX_JOBS] = {0}; // can use bitmap for saving space; 1:- present

void make_job(pid_t pid, char* name){
	if(no_of_jobs >= MAX_JOBS){
		printf("Too much jobs to handle!\n");
		exit(1);
	}
	job_table[no_of_jobs].pid = pid;
	job_table[no_of_jobs].pgid = getpid();
	strcpy(job_table[no_of_jobs].name,name);
	job_status[no_of_jobs] = 1;

	no_of_jobs++;
}

void remove_job(int i){
	// left shift the job table array
	for(int j = i; j < no_of_jobs-1; j++){
		job_table[j].pid = job_table[j+1].pid; 
		job_table[j].pgid = job_table[j+1].pgid;
		strcpy(job_table[j].name,job_table[j+1].name); 
	}
	// update the size of job table
	no_of_jobs--;
}

// check if any jobs is complete
void check_jobs(){
	for(int i = 0; i < no_of_jobs; i++){
		if( !kill(job_table[i].pid,0) )
			continue;
		else{
			// job doesn't exsist
			// remove from job table
			remove_job(i);
		}
	}
}

void show_jobs(){
	printf("\n\tNO\tNAME\t\tPID\n");
	for(int i = 0; i < no_of_jobs; i++){
		if(job_status[i])
			printf("\n\t%d\t%s\t%d\n",i,job_table[i].name,job_table[i].pid);
	}
}

void handle_fg(int id){
	int status;
	// Ignore SIGTTOU 
    signal(SIGTTOU, SIG_IGN);

	// Stop the process 
    kill(-1 * job_table[id].pid, SIGTSTP);

    // Bring the process in foreground 
    tcsetpgrp(STDIN_FILENO, job_table[id].pid);

    /* Continue the process */
    kill(-1 *job_table[id].pid, SIGCONT);

    //Wait for the process to complete 
    waitpid(job_table[id].pid, &status, WUNTRACED);

	if ((WIFEXITED(status)) || (WIFSIGNALED(status))) {
        remove_job(id);
    }
	// Bring the shell in foreground 
    tcsetpgrp(STDIN_FILENO, getpid());
    // Set SIGTTOU back to default 
    signal(SIGTTOU, SIG_DFL);


}

// TODO: Fix pgids
void handle_bg(int id){
	kill(-1 *-1 *job_table[id].pid, SIGCONT);
}

void show_prompt(){
	printf("\nshell$ ");
}

// singal handlers
void handle_SIGINT(){
	printf("\n");
	show_prompt();
	fflush(stdout);
}

// TODO: Assign proper pgid
// Send SIGTSTP to it
// job should be stopped and entry should be made in job table
void handle_SIGTSTP_in_child(){
	// add the process to background
	// make_job(p,job_name);
	// printf("[%d]: stopped + %s\n",no_of_jobs,job_name);
	// kill(-1 * job_table[id].pid, SIGTSTP);
}

void handle_SIGCHLD(){
	// prevent zombie 
    waitpid(-1, NULL, WNOHANG);
}

void init_singals(){
	signal(SIGINT, handle_SIGINT);
	signal(SIGCHLD, handle_SIGCHLD);
    signal(SIGTSTP, handle_SIGINT);
    signal(SIGTTOU, SIG_IGN);
}



char** get_parsed(char *input, char* separator) {
    char** command = (char **)malloc(8 * sizeof(char *));
    char *parsed;
    int index = 0;

    parsed = strtok(input, separator);
    while (parsed != NULL) {
        command[index] = parsed;
        index++;
        parsed = strtok(NULL, separator);
    }

    command[index] = NULL;
    return command;
}

// TODO: implemenet reverse parsing
// char* reverse_parsing(char** command){
	
// }

int main() {
    char cmd[128];
	char** command;
	char* separator;
	char original[128];				// to restore after parsing
	int index;
	int bg_flag;					// for background processes
	// int fg_flag;					// foreground flag
	int id;							// used for id in job table
	char job_name[24];
	while(1){
		init_singals();
		show_prompt();
		scanf("%[^\n]s",cmd);
		getchar();
		bg_flag = 0;
		check_jobs();

		// exit shell
		if( strcmp(cmd,"exit") == 0){
			exit(0);
		}
		if(cmd == NULL){
			exit(0);
		}
		if( strcmp(cmd,"jobs") == 0 ){
			show_jobs();
			continue;
		}
		
		// strtok will modify command; we need to maintain original copy
		strcpy(original,cmd);
		
		// check for background process
		command = get_parsed(cmd,"&");
		if(strcmp(command[0],original) != 0){
			bg_flag = 1;
		}
		strcpy(job_name,command[0]);

		strcpy(cmd,original);
		command = get_parsed(cmd," ");
		if(strcmp(command[0],"fg") == 0){
			// fg_flag = 1;
			if(command[1] == NULL){
				id = 0;
			}
			else{
				id = atoi(command[1]);
			}
			printf("fg id : %d",id);
			if(id < no_of_jobs){
				handle_fg(id);
				continue;
			}
			else{
				printf("\nError: Invalid job number!\n");
				continue;
			}
		}
		if(strcmp(command[0],"bg") == 0){
			// fg_flag = 1;
			if(command[1] == NULL){
				id = 0;
			}
			else{
				id = atoi(command[1]);
			}
			if(id < no_of_jobs){
				handle_bg(id);
				continue;
			}
			else{
				printf("Error: Invalid job number!\n");
				continue;
			}
			
		}
		// restore original cmd
		// strcpy( cmd , reverse_parsing(command) );
		strcpy(cmd,original);
		// reads the '\n' allowing you to type 
		pid_t p = fork();
		if(p < 0){
			printf("Fork failed\n");
		}
		// child -- exec the commands here
		else if(p == 0){
			// set_default_signals();
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			// printf("in fork\n");
			command = get_parsed(cmd,"&");
			if(strcmp(command[0],original) != 0){
				//this command will execute in background
				// bg_flag was set before coming into this fork
				char** cmd1 = get_parsed(command[0]," ");
				if( execvp(cmd1[0],cmd1) < 0){
					printf("exec error : %s\n",cmd1[0]);
					return 1;
				}
			}
			// printf("no &\n");
			//check of redirection
			strcpy(cmd,original);
			command = get_parsed(cmd,">");
			if(strcmp(command[0],original) != 0){
				// contains output redirection
				// assuming only single redirection
				char** cmd1 = get_parsed(command[0]," ");
				char** file = get_parsed(command[1]," ");
				close(1);
				// now fd will get value 1
				// since lowest possible value is assigned and we closed 1 (ie stdout)
				int fd = open(file[0],O_WRONLY|O_CREAT,0777);
				if( execvp(cmd1[0],cmd1) < 0){
					printf("exec error : %s\n",cmd1[0]);
					return 1;
				}
			}
			// printf("no >\n");
			strcpy(cmd,original);
			command = get_parsed(cmd,"<");
			if(strcmp(command[0],original) != 0){
				// contains input redirection
				// assuming only single redirection
				char** cmd1 = get_parsed(command[0]," ");
				char** file = get_parsed(command[1]," ");
				// now cmd1 is subject to execvp
				// file[0] is the file to be read
				// output of second to input of first
				close(0);
				int fd = open(file[0],O_RDONLY);
				if( execvp(cmd1[0],cmd1) < 0){
					printf("exec error : %s\n",cmd1[0]);
					return 1;
				}
			}
			// printf("no <\n");
			// check for pipe
			// handle pipe
			strcpy(cmd,original);
			command = get_parsed(cmd,"|");
			// a | b | c
			if(strcmp(command[0],original) != 0){
				// contains pipe
				// command = {"a","b","c",NULL}
				// count how many commands are there (n)
				int n = 0;
				char* tmp = command[0];
				while(tmp != NULL){
					n++;
					tmp = command[n];
				}
				// printf("Handle %d commands in pipe\n",n);
				// make n-1 pipes
				int pfd[n-1][2];
				// though fork in loop will create many process
				// we concern about only n processes
				int pid[n];

				// open all pipes
				for(int i = 0; i < n-1; i++){
					if(pipe(pfd[i]) < 0){
						printf("Error in pipe!\n");
						return 1;
					}
				}
				// printf("opened %d pipes\n",n-1);
				// get all commands
				char*** cmdptr = (char*** )malloc(sizeof(char**)*n);
				for(int i = 0; i < n; i++){
					cmdptr[i] = get_parsed(command[i]," ");
				}
				// cmdptr[0] = ls with options
				// cmdptr[1] = grep with options
				// cmdptr[2] = tee with options
				for(int i = 0; i < n; i++){
					pid[i] = fork();
					if(pid[i] < 0){
						printf("Error in fork!\n");
						return 1;
					}
					if(pid[i] == 0){
						// child
						// execute cmdptr[i]
						// printf("outer: will execute %s\n",cmdptr[i][0]);
						if(i == 0){
							// first command
							// output to pipe
							// close(1);
							// printf("[%d] process %d will execute %s\n",i,getpid(),cmdptr[i][0]);
							int fd = dup2(pfd[0][1],1);
							// open(fd,O_WRONLY);
							// open(pfd[0][1]);

							// close unused read ends
							for(int j = 0; j < n-1; j++){
								// if(i != j-1 )
									close(pfd[j][0]);
							}
							// close unused write ends
							for(int j = 0; j < n-1; j++){
								if(i != j)
									close(pfd[j][1]);
							}
							
							if( execvp(cmdptr[i][0],cmdptr[i]) < 0){
								printf("exec error!\n");
								return 1;
							}
						}
						else if(i == n-1){
							// last command
							// close(0);
							// printf("[%d] process %d will execute %s\n",i,getpid(),cmdptr[i][0]);
							int fd = dup2(pfd[n-2][0],0);
							// open(fd,O_RDONLY);

							// close all read ends
							for(int j = 0; j < n-1; j++){
								if(i-1 != j)
									close(pfd[j][0]);
							}
							// close unused write ends
							for(int j = 0; j < n-1; j++){
								// if(i != j)
									close(pfd[j][1]);
							}

							if ( execvp(cmdptr[i][0],cmdptr[i]) < 0){
								printf("exec error!\n");
								return 1;
							}
						}
						else{
							// general
							// close unused read ends
							dup2(pfd[i-1][0],0);
							dup2(pfd[i][1],1);
							// printf("[%d] process %d will execute %s\n",i,getpid(),cmdptr[i][0]);
							for(int j = 0; j < n-1; j++){
								if(i-1 != j)
									close(pfd[j][0]);
							}
							// close unused write ends
							for(int j = 0; j < n-1; j++){
								if(i != j)
									close(pfd[j][1]);
							}

							if( execvp(cmdptr[i][0],cmdptr[i]) < 0){
								printf("exec error!\n");
								return 1;
							}
						}
						
					}
				
				}
				int status;
				// for(int k = 0; k < n; k++)
				// waitpid(-1 , &status, WUNTRACED);
				wait(NULL);
				
				return 0;
			}
		
			
			strcpy(cmd,original);
			command = get_parsed(cmd," ");
			if( execvp(command[0],command) < 0){
				printf("exec error! +1\n");
				return 1;
			}
			
		}
		else{
			// parent
			int wstatus;
			// wait for the process group to finish
			// if(fg_flag == 1){
			// 	waitpid(p, &wstatus, WUNTRACED | WCONTINUED);
			// 	fg_flag = 0;
			// }

			if(bg_flag == 0){
				// assign terminal tp process group with child process
				// tcsetpgrp(STDIN_FILENO, p);
				// wait(NULL);
				
				waitpid(p, &wstatus, WUNTRACED | WCONTINUED);
				// allow the shell to come to foreground 
				// signal(SIGTTOU, SIG_IGN);
				// Set the shell to foreground *
				tcsetpgrp(STDIN_FILENO, getpid());
			}
			// if command contained '&'
			// make a entry in job table and continue
			else{
				make_job(p,job_name);
				printf("[%d]: %s\n",no_of_jobs,job_name);
				bg_flag = 0;
			}
		}

	}

    return 0;
}

