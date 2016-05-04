/******************************************************************************
 *
 *  File Name........: main.c
 *
 *  Description......: Simple driver program for ush's parser
 *
 *  Author...........: Vincent W. Freeh
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include "parse.h"

/******** Added by Nitish for micro shell implementation : START********/
char currWorkDir[1024];
int status;
int ctrlCFlag;
int pipefd[20];
int i = 0;
int pipeHandle;
int headCommand = 0;
int oldRCStdIn,rcFile;
char *builtInCommands[] = {"cd","echo","logout","nice","pwd","setenv","unsetenv","where"};
int oldStdIN,oldStdOut,oldStdErr;

void builtInCmdExecutor(Cmd c);

//Saving streams of the main shell
void streamSaver()
{
	fflush(stdin);
	fflush(stdout);
	if((oldStdIN = dup(fileno(stdin))) == -1)
	{
		printf("Saving stdin Streams via dup failed\n");
		return;
	}
	if((oldStdOut = dup(fileno(stdout))) == -1)
	{
		printf("Saving stdout Streams via dup failed\n");
		return;
	}
	if((oldStdErr = dup(fileno(stderr))) == -1)
	{
		printf("Saving stderr Streams via dup failed\n");
		return;
	}
}

//Resetting streams of the main shell
void streamResetter()
{	
	fflush(stdin);
	fflush(stdout);
	if(dup2(oldStdIN,fileno(stdin)) == -1)
	{
		printf("Resetting stdin Streams via dup2 failed\n");
		return;
	}
	close(oldStdIN);
	if(dup2(oldStdOut,fileno(stdout)) == -1)
	{
		printf("Resetting stdout Streams via dup2 failed\n");
		return;
	}
	close(oldStdOut);
	if(dup2(oldStdErr,fileno(stderr)) == -1)
	{
		printf("Resetting stderr Streams via dup2 failed\n");
		return;
	}
	close(oldStdErr);
}

// echo command implementation
void executeEcho(Cmd c)
{
	int i=1;
	if (c->args[1] != NULL){
		if (c->args[1][0] == '$'){
			char *temp = strtok(c->args[1],"$");
			char *result = getenv(temp);
			if (result != NULL)
			  printf("%s", result);
			else
		  	  printf("No such environment variable found");
		}
		else if (!strcmp(c->args[1],"~")){
			char *homeDirPath = (char *)malloc(1000);
	  		strcpy(homeDirPath, getenv("HOME"));
			printf("%s",homeDirPath);
		}
		else{
			while(c->args[i]!=NULL)
			{
				printf("%s ",c->args[i]);
				i++;	
			}
		}
	}	
	printf("\n");
}

// pwd command implementation
void executePwd(){
    getcwd(currWorkDir, sizeof(currWorkDir));
    printf("%s\n", currWorkDir);
}

// setenv command implementation
void executeSetenv(Cmd c)
{	
	int i = 0;
	if(c->args[1] == NULL)
	{
		extern char **environ;
		while(environ[i] != NULL)
		{
			printf("%s\n",environ[i]);
			i++;
		}
		return;
	}
	else
	{
		char* secondArg = (char *)malloc(1000);
		if(c->args[2] == NULL){
                        strcpy(secondArg,"");
		}
		else
		{
			strcpy(secondArg,c->args[2]);
		}
		if(setenv(c->args[1],secondArg,1) == -1){
			printf("Error in setting environment variable\n");
			return;		
		}
	}
}

// unsetenv command implementation
void executeUnsetenv(Cmd c)
{
	if(c->args[1] == NULL)
	{
		printf("No arguments passed for unsetenv.\n");
		return;
	}
	else
	{
	    if (unsetenv(c->args[1]) == -1)
	    {
	   	printf("Error in executing unsetenv.\n");
	   	return;
	    }
 	}
}
// logout command implementation
void executeLogout(Cmd c)
{
	exit(0);
}

// cd command implementation
void executeCD(Cmd c)
{	
	char *homeDirPath = (char *)malloc(1000);
  	strcpy(homeDirPath, getenv("HOME"));
	if(c->args[1] == NULL || !strcmp(c->args[1],"~"))
	{
		c->args[1] = malloc(strlen(homeDirPath));
		strcpy(c->args[1],homeDirPath);
	}
	else if(c->args[1][0] == '~')
	{
		char *temp,temp1[500];
		int tlen = 0;
		temp = strtok(c->args[1],"~");
		if(temp){
			tlen = strlen(temp);
			c->args[1] = malloc(strlen(homeDirPath)+tlen);
			strcpy(c->args[1],strcat(homeDirPath,temp));
		}
	}
        if (chdir(c->args[1]) == -1) {
     		 if(getcwd(currWorkDir, 1024)!=NULL){
        		printf("Error in chdir,Stayed in: %s\n",currWorkDir);
		 }
        }
}

// where command implementation
void executeWhere(Cmd c)
{
	char *path = getenv("PATH");
	char dupPath[500],cmd_path[500]="";
	int i = -1 ;
	int j=0;
	while(j<8){
		if(!strcmp(c->args[1],builtInCommands[j])){
			i = j;
		}
		j++;
	}
	if(i!=-1)
	{
		printf("%s:built-in command\n",builtInCommands[i]);
	}	
	strcpy(dupPath,path);
	char* splittedPath = strtok(dupPath,":");
	while(splittedPath != NULL)
	{
		strcpy(cmd_path,splittedPath);
		strcat(cmd_path,"/");
		strcat(cmd_path,c->args[1]);
		if(!access(cmd_path,F_OK))
		{
		    printf("%s\n",cmd_path);
		}
		splittedPath = strtok(NULL,":");
	}
}

// nice command implementation
void executeNice(Cmd c)
{
	int which, who, priority;
	int setCommand = 0;		
	if(c->args[1]!=NULL)
	{
		priority = atoi(c->args[1]);
		if(priority == 0 && strcmp(c->args[1],"0")){
			priority = 4;
			setCommand = 1;		
		}
		if(priority<-19)
		{
			priority = -19;
		}
		else if(priority>20)
		{
			priority = 20;
		}
	}
	else
	{
		printf("No arguments passed for nice command\n");
	}
	int childPID = fork();
	if(childPID == 0){
		which = PRIO_PROCESS;
		who = 0;	
		getpriority(which, who);
		setpriority(which, who, priority);
		int j = 0;
		int b = 0;		
		while(j<8)
		{
			if(setCommand != 1)
			{
				if(!strcmp(c->args[2],builtInCommands[j]))
				{
					b = 1;		
				}
			}
			else
			{
				if(!strcmp(c->args[1],builtInCommands[j]))
				{
					b = 1;		
				}
			}
			j++;
		}
		if(b){
			Cmd temp;
			temp = malloc(sizeof(struct cmd_t));
			temp->args = malloc(sizeof(char*));
			temp->in = Tnil;
			temp->out = Tnil;
			int i = 2;
			while(c->args[i] != NULL){
				temp->args[i-2] = malloc(strlen(c->args[i]));
				strcpy(temp->args[i-2],c->args[i]);
				i++;
			}
			builtInCmdExecutor(temp);			
			free(temp->args);
			free(temp);
			exit(0);			
		}	
		else{
			if(setCommand)
			{
				if(execvp(c->args[1], c->args+1) == -1){
					fprintf(stderr,"%s: command not found\n",c->args[1]);				
					exit(0);	
				}
			}
			else
			{
				if(execvp(c->args[2], c->args+2) == -1){
					fprintf(stderr,"%s: command not found\n",c->args[2]);				
					exit(0);	
				}
			}
		}
	}
	else{		
		waitpid(childPID,&status,0);
	}
}

// exit command handling
void checkExitCommands(Cmd c) {
  if (!strcmp(c->args[0], "logout") || !strcmp(c->args[0], "quit") || !strcmp(c->args[0], "exit")){
    exit(0);
  }
}

// Common function to call all command handlers
void builtInCmdExecutor(Cmd c)
{
	if(!strcmp(c->args[0],"cd"))
	{
		executeCD(c);
	}
	else if(!strcmp(c->args[0],"echo"))
	{
		executeEcho(c);
	}
	else if(!strcmp(c->args[0],"logout"))
	{
		executeLogout(c);
	}
	
	else if(!strcmp(c->args[0],"nice"))
	{	
		executeNice(c);
	}
	else if(!strcmp(c->args[0],"pwd"))
	{
		executePwd(c);
	}
	else if(!strcmp(c->args[0],"setenv"))	
	{
		executeSetenv(c);
	}	
	else if(!strcmp(c->args[0],"unsetenv"))
	{
		executeUnsetenv(c);
	}
	else if(!strcmp(c->args[0],"where"))
	{
		executeWhere(c);	 
	}
}

//  IO Redirect handling function for the microshell
void ioRedirectHandler(Cmd c) {
	int temp;
	int j = 0;
	int b = 0;		
	while(j<8)
	{
		if(!strcmp(c->args[0],builtInCommands[j]))
		{
			b = 1;		
		}
		j++;
	}

	if(c->in==Tin)
	{
		if((temp = open(c->infile,O_RDONLY)) != -1)
		{
			if(dup2(temp,fileno(stdin)) == -1)
			{
				printf("Input Streams handling via dup2 failed\n");
				return;
			}
			close(temp);
		}
		else
		{
			printf("No such file or directory found\n");
			if(b)
				return;			
			else
				exit(0);
		}
	}

	if(c->out == ToutErr)
	{
		if((temp = open(c->outfile,O_CREAT|O_WRONLY|O_TRUNC,0777)) != -1)
		{	
			if(dup2(temp,fileno(stdout)) == -1)
			{
				printf("Output Streams handling via dup2 failed\n");
				return;
			}
			if(dup2(temp,fileno(stderr)) == -1)
			{
				printf("Error Streams handling via dup2 failed\n");
				return;
			}
			close(temp);
		}
		else
		{
			printf("No such file or directory found\n");
			if(b)
				return;			
			else
				exit(0);
		}
	}
	else if(c->out == Tout)
	{
		if((temp = open(c->outfile,O_CREAT|O_WRONLY|O_TRUNC,0777)) != -1)
		{	
			if(dup2(temp,fileno(stdout)) == -1)
			{
				printf("Output Streams handling via dup2 failed\n");
				return;
			}
			close(temp);
		}
		else
		{
			printf("No such file or directory found\n");
			if(b)
				return;			
			else
				exit(0);
		}
	}
	else if(c->out == TappErr)
	{
		if((temp = open(c->outfile,O_CREAT|O_WRONLY|O_APPEND,0777)) != -1)
		{	
			if(dup2(temp,fileno(stdout)) == -1)
			{
				printf("Output Streams handling via dup2 failed\n");
				return;
			}
			if(dup2(temp,fileno(stderr)) == -1)
			{
				printf("Error Streams handling via dup2 failed\n");
				return;
			}
			close(temp);
		}		
		else
		{
			printf("No such file or directory found\n");
			if(b)
				return;			
			else
				exit(0);
		}
	}
	else if(c->out == Tapp)
	{		
		if((temp = open(c->outfile,O_CREAT|O_WRONLY|O_APPEND,0777)) != -1)
		{	
			if(dup2(temp,fileno(stdout)) == -1)
			{
				printf("Error Streams handling via dup2 failed\n");
				return;
			}
			close(temp);
		}
		else
		{
			printf("No such file or directory found\n");
			if(b)
				return;			
			else
				exit(0);
		}
	}
}	

// Driver function for the microshell
void shellHandler(Cmd c) {
        streamSaver();
	
	if(c->out == Tpipe)
	{
		if (pipe(pipefd+i) == -1) {
               		printf("Error in creating pipe");
			return;
		}
		else
		{			
			pipeHandle = 1;	
		}
		i = i + 2;
	}
	int j = 0;
	int b = 0;		
	while(j<8)
	{
		if(!strcmp(c->args[0],builtInCommands[j]))
		{
			b = 1;		
		}
		j++;
	}

	if(b){
		if(c->out == Tpipe)
		{
			if (dup2(pipefd[i-1],fileno(stdout)) == -1)
			{
				printf("Error in redirecting output from pipe\n");
				return;			
			}
		}
		else if (c->out == TpipeErr)
		{
			if (dup2(pipefd[i-1],fileno(stdout)) == -1)
			{
				printf("Error in redirecting output from pipe\n");
				return;			
			}
			if (dup2(pipefd[i-1],fileno(stderr)) == -1)
			{
				printf("Error in redirecting output from pipe\n");
				return;			
			}
		}

		if(pipeHandle && !headCommand)
		{			
			if(c->out == Tpipe || c->out == TpipeErr)
			{	
				if (dup2(pipefd[i-4],fileno(stdin)) == -1)
				{
					printf("Error in redirecting input to pipe\n");	
					return;		
				}
			}
			else
			{
				if (dup2(pipefd[i-2],fileno(stdin)) == -1)
				{
					printf("Error in redirecting input to pipe\n");	
					return;		
				}
			}	
		}
                ioRedirectHandler(c);
		builtInCmdExecutor(c);
		streamResetter();
					
	}	
	else{
		int childPID = fork();
		if(childPID == 0){
			if(c->out == Tpipe)
			{
				if (dup2(pipefd[i-1],fileno(stdout)) == -1)
				{
					printf("Error in redirecting output from pipe\n");	
					exit(0);		
				}
			}
			else if (c->out == TpipeErr)
			{
				if (dup2(pipefd[i-1],fileno(stdout)) == -1)
				{
					printf("Error in redirecting output from pipe\n");
					exit(0);			
				}
				if (dup2(pipefd[i-1],fileno(stderr)) == -1)
				{
					printf("Error in redirecting output from pipe\n");
					exit(0);			
				}
			}

			ioRedirectHandler(c);	
			if(pipeHandle && !headCommand)
			{			
				if(c->out == Tpipe || c->out == TpipeErr)
				{	
					if (dup2(pipefd[i-4],fileno(stdin)) == -1)
					{
						printf("Error in redirecting input to pipe\n");
						exit(0);			
					}
				}
				else
				{
					if (dup2(pipefd[i-2],fileno(stdin)) == -1)
					{
						printf("Error in redirecting input to pipe\n");	
						exit(0);		
					}
				}	
			}
			if(execvp(c->args[0], c->args) == -1){
				fprintf(stderr,"%s: command not found\n",c->args[0]);				
				exit(0);	
			}
			exit(0);				
		}
		else{
			checkExitCommands(c);
			waitpid(childPID,&status,0);
			headCommand = 0;
			if(pipeHandle){
				close(pipefd[i-1]);
				if(i-4 >= 0){
				       	close(pipefd[i-4]);
				}	
			}
		}
	}
	headCommand = 0;
	if(pipeHandle){
		close(pipefd[i-1]);
		if(i-4 >= 0){
		       	close(pipefd[i-4]);
		}	
	}
}

//Command handler starting point after main
static void prPipe(Pipe p)
{
  Cmd c;
  if (p == NULL)
    return;
  headCommand = 1;
  for ( c = p->head; c != NULL; c = c->next ) {
	checkExitCommands(c);
	shellHandler(c); 
  }
  i = 0;
  pipeHandle = 0;
  prPipe(p->next);
 }


//ushrc execution handling
void ushrcProcessing()
{
  char *rcpath = (char *)malloc(1000);
  strcpy(rcpath, getenv("HOME"));
  rcpath = strcat(rcpath, "/.ushrc");
  if((rcFile = open(rcpath,O_RDONLY)) != -1)
  {
	oldRCStdIn = dup(fileno(stdin));
	if(oldRCStdIn == -1)
	{
		printf("dup rcfile failed");
		return;
	}
	if(dup2(rcFile,fileno(stdin))==-1)
	{
		printf("dup rcfile failed");
		return;
	}
  }
  else
  {
	printf("No .ushrc file found or file permission denied\n");
	return;
  }
  Pipe p;
  char *host = (char *)malloc(1000);
  gethostname(host,1024);
  while(1){
    
    fflush(stdin);
    if(isatty(fileno(stdin))){
       printf("%s%%", host);
    }
    p = parse();
    if(p == NULL)
    {
	continue;
    }
    if(p!=NULL && strcmp(p->head->args[0], "end"))
    {
    	prPipe(p);
    }
    else if(isatty(fileno(stdin)) && !strcmp(p->head->args[0], "end"))
    {
	exit(0);
    }	
    else if(!isatty(fileno(stdin)) && !strcmp(p->head->args[0], "end")){
	break;
    }
    freePipe(p);
  }
  dup2(oldRCStdIn,fileno(stdin));
  close(rcFile);
  close(oldRCStdIn);
}

// Ctrl+C signal handler function
void hostDisplay()
{
        char *host = (char *)malloc(1000);
  	gethostname(host,1024);
  	printf("\n%s%%", host);	
        fflush(stdout);
	ctrlCFlag = 0;
}

//Signal handling for keyboard interrupts
void signalHandling()
{
	signal (SIGQUIT, SIG_IGN);
	signal (SIGTERM, SIG_IGN);
	signal (SIGINT, SIG_IGN);
	signal (SIGKILL, SIG_IGN);
	signal (SIGHUP, SIG_IGN);
      	signal (SIGINT, hostDisplay);
      	signal (SIGTSTP, SIG_IGN);
}

//Entry point of the P2- microshell implementation
int main(int argc, char *argv[])
{
  Pipe p;
  ctrlCFlag = 1;
  signalHandling();
  ushrcProcessing();
  char *hostName = (char *)malloc(1000);
  gethostname(hostName,1024);    
  while(1){
    fflush(stdin);
    if(isatty(fileno(stdin)) && ctrlCFlag == 1){
       printf("%s%%", hostName);
       fflush(stdout);
    }
    ctrlCFlag = 1;
    p = parse();
    if(p == NULL)
    {
	continue;
    }
    if(p!=NULL && strcmp(p->head->args[0], "end"))
    {
    	prPipe(p);
    }
    else if(isatty(fileno(stdin)) && !strcmp(p->head->args[0], "end"))
    {
	exit(0);
    }	
    else if(!isatty(fileno(stdin)) && !strcmp(p->head->args[0], "end")){
	break;
    }
    freePipe(p);
  }
}
/******** Added by Nitish for micro shell implementation : END ********/
/*........................ end of main.c ....................................*/
