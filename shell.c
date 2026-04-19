#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <stdlib.h> // EXIT_SUCCESS
#include <signal.h> //kill
#include <string.h> //strlen
#include <fcntl.h> //open
//#include "work_with_audio.h" //remove before the next step 
#include "shell.h"

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;
static int pid_fg;//process, that running in the foreground
static int pid_bg_cnt;//how_many_processes in the background
static int pid_bg[MAXCMDS];
static int stopped_pid[MAXCMDS];
static int stopped_pid_cnt;
static pid_t main_pid;

static void handle_sigint(int signal);
static void handle_sigtstp(int signal);
static int work_with_io(int i,int ncmds);

int main(int argc,char *argv[])
{
	register int i;
	char line[1024]; /*allow large command lines*/
	int ncmds;
	char prompt[50];/*shell prompt*/
	int status_pid; /* check program state in wait */
	main_pid=getpid();
	pid_bg_cnt=0;
	stopped_pid_cnt=0;
	
	/* PLACE SIGNAL CODE HERE */
	// struct sigaction sa_ign;
	// sa_ign.sa_handler = SIG_IGN;
	// sa_ign.sa_flags = 0;

	struct sigaction sa;
	sa.sa_handler = handle_sigint;
	sa.sa_flags=SA_RESTART;
	if (sigaction(SIGINT,&sa,NULL) == -1) //0 if success; -1 if unsuccess
	{
		perror("main process sigaction");
		return MAIN_SIGACTION_FAIL;
	}
	

	sprintf(prompt,"[%s]", argv[0]);
	while (promptline(prompt, line, sizeof(line)) > 0) { /* until eof */
		#ifdef DEBUG
		fprintf(stdout,"1ncmds=%d\n",ncmds);
		#endif
		if((ncmds = parseline(line)) <= 0)
			continue;
		#ifdef DEBUG
		fprintf(stdout,"2ncmds=%d\n",ncmds);
		{
			int i, j;
			for (i = 0; i < ncmds; i++) {
			for (j = 0; cmds[i].cmdargs[j] != (char *) NULL; j++){
				fprintf(stderr, "cmds[%d].cmdargs[%d] = %s\n", i, j, cmds[i].cmdargs[j]);
				int len_arg=strlen(cmds[i].cmdargs[j]);
				for(int k=0;k<len_arg;k++)
					fprintf(stderr, "%d ", cmds[i].cmdargs[j][k]);
				fprintf(stderr, "\n");
				
				}
			fprintf(stderr, "cmds[%d].cmdflag = %o\n", i, cmds[i].cmdflag);
			
			fprintf(stderr, "ncmds=%d\n", ncmds);
			fprintf(stderr, "now PID=%d\nis main=%d\n", getpid(),getpid()==main_pid);
			
			}
		}
		#endif
		for (i = 0; i < ncmds; i++) 
		{
			/* FORK AND EXECUTE */
			#ifdef DEBUG
			fprintf(stderr, "before fork\n");
			#endif
			pid_t child_pid = fork();
			if (child_pid == -1) {
				perror("fork");
				return FORK_FAILURE;
			}
			#ifdef DEBUG
			fprintf(stderr, "after fork\n");
			#endif
			if (getpid()!=main_pid) 
			{
				#ifdef DEBUG
				fprintf(stderr, "PID=%d\nchildPID=%d\nncmds=%d\ti=%d\n",getpid(),child_pid,ncmds,i);
				fflush(stdout);
				#endif
				setpgid(0,0);
				int err_check_io = work_with_io(i,ncmds);
				if (err_check_io)
					return err_check_io;
				struct sigaction sa_child;
				sa_child.sa_handler = SIG_IGN;
				sa_child.sa_flags=0;
				if (sigaction(SIGINT,&sa_child,NULL) == -1)
				{
					perror("child process sigaction sigint");
					return CHILD_SIGACTION_FAIL;
				}
				if (sigaction(SIGQUIT,&sa_child,NULL) == -1)
				{
					perror("child process sigaction sigquit");
					return CHILD_SIGACTION_FAIL;
				}
				struct sigaction sa_tstp;
				sa_tstp.sa_handler=handle_sigtstp;
				sa_tstp.sa_flags=0;
				if (sigaction(SIGTSTP,&sa_tstp,NULL) == -1) //0 if success; -1 if unsuccess
				{
					perror("child process sigaction_tstp");
					return CHILD_SIGACTION_FAIL;
				}
				/*
				if (cmds[i].cmdargs[0][0] =='b' && *cmds[i].cmdargs[0][1] == 'g'){
					if (stopped_pid_cnt!=0){
						stopped_pid_cnt
					}
				}*/


				execvp(cmds[i].cmdargs[0],cmds[i].cmdargs);

				fprintf(stdout, "Program not found:%s.\n",cmds[i].cmdargs[0]);
				perror("execvp");
				return EXECVP_FAILURE;

			}
		else if (!bkgrnd)
			{
				
				struct sigaction sa_tstp_main;
				sa_tstp_main.sa_handler=SIG_IGN;
				sa_tstp_main.sa_flags=0;
				if (sigaction(SIGTSTP,&sa_tstp_main,NULL) == -1) //0 if success; -1 if unsuccess
				{
					perror("main process sigaction_tstp");
					return MAIN_SIGACTION_FAIL;
				}
				#ifdef DEBUG
				fprintf(stderr, "!bkgrnd\n");
				#endif
				// if(tcsetpgrp(STDIN_FILENO,child_pid) == -1){
				// 	perror("tcsetprgrp_in child_");
				// }
				// if(tcsetpgrp(STDOUT_FILENO,child_pid) == -1){
				// 	perror("tcsetprgrp_out child_");
				// }	
				#ifdef DEBUG
				fprintf(stderr, "after tcsetprgrp\n");
				#endif
				pid_fg = child_pid;
				/*if (sigaction(SIGTSTP,&sa_tstp,NULL) == -1){
					perror("sigaction tstp");
				}*/
				waitpid(child_pid,&status_pid,0);
				if (WIFEXITED(status_pid)){
					int exit_status = WEXITSTATUS(status_pid);
					if (exit_status != 0){
						fprintf(stderr,"Child failed with code: %d\n",exit_status);
					}
				}
				// if(tcsetpgrp(STDIN_FILENO, main_pid) == -1){
				// 	perror("tcsetprgrp_in main_");
				// }
				// if(tcsetpgrp(STDOUT_FILENO,main_pid) == -1){
				// 	perror("tcsetprgrp_out main_");
				// }

				if (pid_fg==-1) {
					pid_fg = 0;
					break;
				}
				else
					pid_fg=0;
			}
			else
			{
				if (waitpid(child_pid,NULL,WNOHANG) == -1){
					perror("waitpid_wnohang");
					return WAITPID_FAILURE;
				}
				fprintf(stdout, "PID=%d now running in the background\n",child_pid);
				#ifdef DEBUG
				fprintf(stderr, "\npid_bg_cnt=%d; current pid=%d,child_pid=%d;\n",pid_bg_cnt,getpid(),child_pid);
				#endif
				if (pid_bg_cnt<MAXCMDS-1) 
					pid_bg[pid_bg_cnt++]=child_pid;
				#ifdef DEBUG
				fprintf(stderr, "\n0pid_bg_cnt=%d\n",pid_bg_cnt);
				#endif
			}
		}
		for(int for_p=0;for_p<pid_bg_cnt;for_p++)//check all processes if they finished
		{
			int  exit_waitpid=waitpid(pid_bg[for_p],&status_pid,WNOHANG);
			#ifdef DEBUG
			fprintf(stderr, "\n2pid_bg_cnt=%d\tWIFEXITED=%d\tpid_bg[%d]=%d\n",pid_bg_cnt,WIFEXITED(status_pid),for_p,pid_bg[for_p]);
			#endif
			if (exit_waitpid!=-1 && WIFEXITED(status_pid) != 0) {
				pid_bg[for_p]=pid_bg[pid_bg_cnt--];		
			} 
			else if (exit_waitpid==-1 && pid_bg_cnt>=1){
				pid_bg[for_p]=pid_bg[pid_bg_cnt--];		
			}
			#ifdef DEBUG
			fprintf(stderr, "\nafter pid_bg_cnt=%d\n",pid_bg_cnt);
			#endif
			
		}
	}/* close while */
	return 0;
}
/* PLACE SIGNAL CODE HERE */
static void handle_sigint(int signal)
{
	while(waitpid(-1,NULL,WNOHANG)>0);
	if (pid_fg>0)
	{
		if (kill(pid_fg,SIGTERM) == -1) //0 if success; -1 if failure
		{
			perror("handle_sigint kill");
			return;
		}
		if(waitpid(pid_fg,NULL,0) == -1)// 0 if WNOHANG; 
								// child_pid whose child state changed
								// -1 if failure
		{
			perror("handle_sigint waitpid");
			return;
		}
		pid_fg= -1; //process was stopped
	}
	fprintf(stdout,"\n");
	return;
}

static void handle_sigtstp(int signal)
{
	if (pid_fg>0)
	{
		if (kill(pid_fg,SIGTSTP) == -1) //0 if success; -1 if failure
		{
			perror("handle_sigtstp stop");
			return;
		}
		fprintf(stdout,"\nProcess:%d paused.\n",pid_fg);
		stopped_pid[stopped_pid_cnt]= pid_fg;
		if (stopped_pid_cnt<MAXCMDS-1)
			stopped_pid_cnt++;
		pid_fg= -1; //process was stopped
		// if(tcsetpgrp(STDIN_FILENO, main_pid) == -1){
		// 	perror("tcsetprgrp_in main_");
		// }
		// if(tcsetpgrp(STDOUT_FILENO,main_pid) == -1){
		// 	perror("tcsetprgrp_out main_");
		// }
	}
	return;
}


static int work_with_io(int i,int ncmds){
	
	if (infile&& i==0){
		int file = open(infile, O_CREAT | O_RDWR, 0777);
		if (file == -1)
		{
			perror("open");
			return FILE_OPEN_FAILURE;
		}
		if (dup2(file, STDIN_FILENO) == -1)
		{
			perror("dup2");
			return DUP_FAILURE;
		}
		if (close(file) == -1)
		{
			perror("close");
			return CLOSE_FAILURE;
		}
	}
	if (outfile && i==ncmds-1){
		int file = open(outfile, O_CREAT | O_RDWR, 0777);
		if (file == -1)
		{
			perror("open");
			return FILE_OPEN_FAILURE;
		}
		if (dup2(file, STDOUT_FILENO) == -1)
		{
			perror("dup2");
			return DUP_FAILURE;
		}
		if (close(file) == -1)
		{
			perror("close");
			return CLOSE_FAILURE;
		}
	}
	if (appfile && i==ncmds-1){
		int file = open(appfile, O_CREAT | O_RDWR | O_APPEND, 0777);//add text to the end of the file
		if (file == -1)
		{
			perror("open");
			return FILE_OPEN_FAILURE;
		}
		if (dup2(file, STDOUT_FILENO) == -1)
		{
			perror("dup2");
			return DUP_FAILURE;
		}
		if (close(file) == -1)
		{
			perror("close");
			return CLOSE_FAILURE;
		}
	}
	return 0;
}
