#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

#define BUFFER_SIZE 50

static char buffer[BUFFER_SIZE];
/* buffer index to cycle through buffer when printing history */
int historyIndex = 0;
/* array of the past 10 commands */
char history[10][80];

static int numCommands = 1;		/* number of commands entered  */

/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a
 * null-terminated string.
 */

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
   
    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE); 

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
    if (length < 0){
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    /* examine every character in the inputBuffer */
    for (i=0;i<length;i++) {
        switch (inputBuffer[i]){
          case ' ':
          case '\t' :               /* argument separators */
            if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;
          case '\n':                 /* should be the final char examined */
            if (start != -1){
                    args[ct] = &inputBuffer[start];    
                ct++;
            }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
            break;
          default :             /* some other character */
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&'){
                *background  = 1;
                start = -1;
                inputBuffer[i] = '\0';
            }
          }
     }   
     args[ct] = NULL; /* just in case the input line was > 80 */
}


void handle_SIGTSTP() {
   	
   	/* output buffer for what will be printed onto the screen for history */
   	char outputBuffer[BUFFER_SIZE] = "";
   	
   	/* used to number the last commands in history */
   	int numHistory = numCommands-10;
   	
   	/* write a newline character */
	write(STDOUT_FILENO,"\n",strlen("\n"));
   	
	for (int i=0; i<10; i++) {
		
		/* write the number of the commands before each command */ 	
		sprintf(outputBuffer + strlen(outputBuffer), "%d ", numHistory+i);
		
		strcat(outputBuffer, history[historyIndex]);
		strcat(outputBuffer, "\n");
		
		/* write each element of history onto console */
		if (numHistory+i > 0) {
			write(STDOUT_FILENO,outputBuffer,strlen(outputBuffer));
		}
		
		/* increment historyIndex, but cycle through numbers 0-9 */
		historyIndex = ((historyIndex + 1) % 10);
		
		/* clear output buffer */
		strcpy(outputBuffer, "");
		
	}
}


int main(void)
{
char inputBuffer[MAX_LINE];      /* buffer to hold the command entered */
    int background;              /* equals 1 if a command is followed by '&' */
    char *args[(MAX_LINE/2)+1];  /* command line (of 80) has max of 40 arguments */

    
 /* set up the signal handler */
    struct sigaction handler;
    handler.sa_handler = handle_SIGTSTP;
    handler.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &handler, NULL);
    
    /* startup message */
    printf("Welcome to jcshell. My pid is %d.\n", getpid());

    while (1){            /* Program terminates normally inside setup */
        background = 0;
        printf("jcshell[%d]: ", numCommands);
        /* this line prevents needing a newline character before the command */
        fflush(stdout);
        setup(inputBuffer,args,&background);       /* get next command */
        /* increment command count by 1 */
        numCommands++;
        
        /* add each element of arg into the current history[historyIndex] */
        int x = 0;
        for (int i=0; args[i] != NULL; i++) {
       		int j = 0;
       		while (args[i][j] != '\0') {
       			history[historyIndex][x] = args[i][j];
       			j++;
       			x++;
       		}
       		history[historyIndex][x] = ' ';
       		x++;
       	}
       	historyIndex = ((historyIndex + 1) % 10);

      	/* the steps are:
       	(0) if built-in command, handle internally
       	(1) if not, fork a child process using fork()
       	(2) the child process will invoke execvp()
       	(3) if background == 0, the parent will wait,
            otherwise returns to the setup() function. */
       
       	/* if the argument is yell, perform the action necessary */
       	if (strcmp(args[0], "yell") == 0) {
       		
       		for (int i=1; args[i] != NULL; i++) {
       			int j = 0;
       			while (args[i][j] != '\0') {
       				args[i][j] = toupper(args[i][j]);
       				j++;
       			}
       			printf("%s ", args[i]); 
       		}
       		
       		printf("\n");
       		
       	}
       	/* exit */
       	else if (strcmp(args[0], "exit") == 0) {
       		pid_t shellPid = getpid();
       		
       		char exitInfo[80];
       		snprintf(exitInfo, sizeof(exitInfo), "ps -o pid,ppid,pcpu,pmem,etime,user,command -p %d", shellPid);
       		
       		system(exitInfo);
       		exit(0);
       	}
       	/* reapeat command from history (not actually implemented) */
       	else if (strcmp(args[0], "r") == 0) {
       		/* if there are no other arguments, repeat the last command */
       		if (args[1] == NULL) {
       			printf("%s\n", history[(historyIndex+8)%10]);
       		}
       		/* otherwise, repeat the command indicated with the next argument */
       		else {
       			/* cast second argument into an integer */
       			int numArg = atoi(args[1]); 
       			printf("Number of command to execute: %d\n", numArg);
       		}
       	}
       	else {
       		pid_t pid = fork(); /* fork off a child process */
       		
       		/* check if the pid is 0 (child) or other (parent/error) */ 
 			if (pid == 0) { /* child process */
 				execvp(args[0], args);
 			}
 			else if (pid < 0) { /* error occurred */
 				fprintf(stderr, "Fork failed.");
 				return 1;
 			}
 			else { /* parent process */
 				/* check if parent should wait by checking background value */
 				/* string to represent TRUE or FALSE */
 				char flag[6] = "FALSE";
 				
 				if (background == 1) {
 					strcpy(flag, "TRUE");
 				}
 				
 				printf("Child pid = %d, background = %s\n", pid, flag);
 				if (background == 0) {
 					waitpid(NULL, pid);
 					printf("Child process complete\n");
 				}
 			}
       		
       	}
    }
}
