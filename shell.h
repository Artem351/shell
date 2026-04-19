#define MAXARGS 256
#define MAXCMDS 50

struct command {
	char *cmdargs[MAXARGS];
	char cmdflag;
};

/*cmdflag's*/
#define OUTPIP 01
#define INPIP  02

extern struct command cmds[];
extern char *infile, *outfile, *appfile;
extern char bkgrnd;

int parseline(char *);
int promptline(char *, char *, int);

#define DEBUG
//#define DEBUG_PARSE

#define MAIN_SIGACTION_FAIL 1
#define FORK_FAILURE 2
#define EXECVP_FAILURE 3
#define FILE_OPEN_FAILURE 4
#define DUP_FAILURE 5
#define CLOSE_FAILURE 6
#define CHILD_SIGACTION_FAIL 7
#define WAITPID_FAILURE 8
