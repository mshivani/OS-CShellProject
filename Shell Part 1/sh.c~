#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <glob.h>
#include "sh.h"
typedef int bool;
#define TRUE 1
#define FALSE 0

#define BUFSIZE 64
#define DELIM " \t\r\n\a"
int MAX_LENGTH = 256;
int HISTORY_MAX_SIZE;
struct Node *head, *tail;
char buf[512];    




int sh(int argc, char **argv, char **envp )
{
  
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *prompt2 = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;
  char *line;
  char *aliasCheck = calloc(PROMPTMAX, sizeof(char));
  char **aliasArgs  = calloc(MAXARGS, sizeof(char*));
  int aliasArgsCount = 0;
  glob_t globbuf;
  globbuf.gl_offs = 1;
  
 
  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;		/* Home directory to start
						  out with*/

  //signal(SIGINT, signal_callback_handler);
     
  if ((pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';


  signal(SIGINT, sigintHandler);
  signal(SIGTSTP, sigintHandler);
  signal(SIGTERM, sigintHandler);
  
  
  /* Put PATH into a linked list */
  pathlist = get_path();
  prompt = "[cwd]>";
  prompt2 = "[cwd]>";
  
  while ( go )
    {
      /* print your prompt */
      printf("%s",prompt);
      
      /* get command line and process */
      command = read_line();
      int c = 0;
      bool wildcard = FALSE;
      while(command[c]!=NULL){
	if(command[c]== '*' || command[c]== '?'){
	  wildcard = TRUE;
	}
	c++;
      }
      PlaceInBack(command);
      args = split_line(command);
      argv = args;

	/*acount keeps track of the alias mapping. 
	  Even numbers are the shortcuts, odd numbers are the commands */ 
      int acount = 0;
	/*while loop that checks for the shortcut, if found finds the command associated with it*/
      while(aliasArgs[acount+1]!=NULL){
	if(strcmp(args[0], aliasArgs[acount])==0){
	  strcpy(aliasCheck, aliasArgs[acount+1]);
	}
	else{
	   strcpy(aliasCheck, " ");
	}
	acount = acount +2;
        //	printf("%s\n", aliasArgs[acount]);
	//	acount++;
      }
      
      
	/*position gives us the number of args input into command*/
      int position = 0;
      while(args[position]!= NULL){
	position++;
      }
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//      
//	Check for each built in command and implement. 							    //
//	Every command checks for the built in, and checks the alias list for shortcut.	  		    //
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
	/* exit command, prints an exit statement then closes shell with exit(0)*/
      if (strcmp(command, "exit")==0 || strcmp(aliasCheck, "exit")==0){
	printf("Executing built in command %s\n", args[0]);
	exit(0);
      }/* end of exit*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//      
	/* which command, requires two arguements minimum */
      else if (strcmp(command, "which")==0 || strcmp(aliasCheck, "which")==0){
	printf("Executing built in command %s\n", args[0]);
	//error checking for only one argument 
	if(position == 1){
	  printf("which: Too few arguments.\n");
      }
	// goes through calls which on the arguments
	else{
	  int pos = 1;
	  while(args[pos]!= NULL){
	    which(args[pos], pathlist);
	    pos++;
	}
	}
      }/* end of which*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
	/* Where command, needs two arguments*/
      else if (strcmp(command, "where")==0 || strcmp(aliasCheck, "where")==0){
        printf("Executing built in command %s\n", args[0]);
	//error checking for only one argument
	if(position == 1){
	  printf("where: Too few arguments.\n");
	}
	//calls where on other arguments
	else{
	  int pos2 = 1;
	  while(args[pos2]!= NULL){
	    where(args[pos2], pathlist);
	  pos2++;
	  }
	}
      }/* end of where*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//      
	/* CD command, can have no arguments. redirects user to different directories*/       
      else if (strcmp(command, "cd")==0|| strcmp(aliasCheck, "cd")==0){
        printf("Executing built in command %s\n", args[0]);
	// if no arguments other than cd, sends to HOME directory
	if(position==1){
	  getcwd(owd, PATH_MAX);
	  chdir(getenv("HOME"));
	}
	// if "cd -" then sets owd
	else if((position==2) && strcmp(args[1],"-")==0){
	  chdir(owd);
	}
	// otherwise look for the directory to move to
	else if((position==2) && strcmp(args[1],"-")!=0){
	  char *directory = args[1];
	  int check;
	  check = chdir (directory);
	  if(check==0){
	    getcwd(owd, PATH_MAX);
	  }
	// if no directory found, perror "directory does not exist"
	  else{
	    perror("No such file or directory\n");
	  }
	}
	// if too many arguments are found, perror "Too many args"
      else{
	perror("cd: Too many args\n");
      }
      }/* end of cd*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//      
	/* PWD command, displays current directory so long as pwd is the first argument*/
      else if (strcmp(command, "pwd")==0 || strcmp(aliasCheck, "pwd")==0){
        printf("Executing built in command %s\n", args[0]);
	printf("%s\n",getcwd(pwd, PATH_MAX));
      }/* end of pwd*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//    	
	/* List command, prints out all files in directory*/
      else if (strcmp(command, "list")==0 || strcmp(aliasCheck, "list")==0){
	printf("Executing built in command %s\n", args[0]);
	char *listall = ".";
	if(position == 1){
	  //calls the list function that is implemented below and prints all files in directory
	  list(listall);
	}
	else {
	   //calls the list function that is implemented below
	  //and prints files requested
	  list(args[1]);
	}    
	
      }

    /* end of List*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//   
	/* PID command, prints out commands process ID*/   
      else if (strcmp(command, "pid")==0 || strcmp(aliasCheck, "pid")==0){
        printf("Executing built in command %s\n", args[0]);
	printf("%d\n",getpid());
      }/* end of pid()*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
	/* kill command, kills the process indicated*/
      else if(strcmp(command, "kill")==0 || strcmp(aliasCheck, "kill")==0){
	printf("Executing built in command %s\n", args[0]);
	//needs to have at least one argument
	if(position == 1){
	  perror("kill: Too few arguments.\n");
	}
	//sends SIGTERM to given argument
	if(position == 2){
	  kill(atoi(args[1]), SIGTERM);
	}
	
	if(position == 3){
	  int temp = args[1][1] - '0';
	  kill(atoi(args[2]), temp);
	}
      }/* end of kill*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//   
	/*Prompt command, When prompt is called, will put the argument as a prefix to the command line */   
      else if (strcmp(command, "prompt")==0 || strcmp(aliasCheck, "prompt")==0){
	char *read;
	char **arguments;
	//if prompt is enter alone, will ask you to enter a prefix
	if(position == 1){
	  printf("input prompt prefix: ");
	  read  = read_line();
	  arguments = split_line(read);
	  prompt = strcat(arguments[0], " ");
	  prompt = strcat(prompt, prompt2);
	}
	// otherwise place first argument after prompt as prefix
	else if (position >1){
	  prompt = strcat(args[1], " ");
	  prompt = strcat(prompt, prompt2);
	}
      }/* end of prompt*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------// 
	/* printenv, prints out the specified enviornment variable. If no variable is specified, 
	   print out all enviornment variables*/     
      else if (strcmp(command, "printenv")==0 || strcmp(aliasCheck, "printenv")==0){
        printf("Executing built in command %s\n", args[0]);
	//if only printenv is called, prints all enviornments
	if(position ==1){
	  char** env = envp;
	  while (*env != 0)
	    {
	      printf("%s\n", *env);
	      env++;
	    }
	}
	//prints the desired enviornment variable
	else if(position == 2){
	  printf("%s\n",getenv(args[1]));
	}
	//otherwise sends an error
	else {
	  perror("error\n");
	}
      }/* end of printenv*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------// 
	/* Alias command, allows for user input to rename references and allow the reference to be accessed
	   by using the shortcut name*/     
      else if (strcmp(command, "alias")==0 || strcmp(aliasCheck, "alias")==0){
        printf("Executing built in command %s\n", args[0]);
	//so long as at least alias and another argument are input, then the values are stored in the char array 
	if(position > 2){
	  aliasArgs[aliasArgsCount] = args[1];
	  aliasArgsCount++;
	  aliasArgs[aliasArgsCount] = args[2];
	  aliasArgsCount++;
	}
	// prints out the mapping of command to alias
	else if(position == 1){
	  int cnt = 0;
	  while( cnt < aliasArgsCount){
	    printf("%s\t%s\n", aliasArgs[cnt], aliasArgs[cnt+1]);
	    cnt = cnt+2;
	  }
	}
      }/* end of alias*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//      
	/* History Command, will show previously typed commands using a singly inked list*/
      else if (strcmp(command, "history")==0 || strcmp(aliasCheck, "history")==0){
        printf("Executing built in command %s\n", args[0]);
      int count = 0;        // counter
      int printCount = 0;   // the number of nodes to print out 
      int numNodes = 0;     // number of nodes in linked list
      //create 2 temp Nodes representing the head of the list
	struct Node* temp = head;	//Uses one temp to traverse linked list during if and else statements
	struct Node* temp2 = head;	//Uses one temp traverse the nodes and get a count of the nodes.
	//get the number of nodes in the linked list
	while(temp2 != NULL) {
	  numNodes++;
	  temp2 = temp2->next;
	}
	// if history is typed in without arguments, prints last 10 commands
	if(position == 1){
	while(temp != NULL && count < 10) {  // loops through list. When it is 10 away from the end  
	  if(numNodes - 10 <= printCount){   // starts printing the commands in order
	    printf("%s",temp->command);
	      count++;
	    }
	    temp = temp->next;
	    printCount++;
	  }	
	}
	// if something other than a number is put in will return an error
	else if(position == 2){
	  if(atoi(args[1]) == NULL){
	    perror("Badly formed Number");
	  }
	  else{
	    while(temp != NULL && count < atoi(args[1])) {
	      if(numNodes - atoi(args[1]) <= printCount){
		printf("%s",temp->command);
		count++;
	      }
	      temp = temp->next;
	      printCount++;
	  }
	  }
	}
      }    /* end of history*/    
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//      
      /*Setenv command, sets an enviornments name. */
      else if (strcmp(command, "setenv")==0 || strcmp(aliasCheck, "setenv")==0){
        printf("Executing built in command %s\n", args[0]);
	//if no arguments given to setenv, then all enviornment variables are displayed
	if(position ==1){
	  char** env = envp;
	  while (*env != 0)
	    {
	      printf("%s\n", *env);
	      env++;
	    }
	}
	//if only one arguement is found, sets that enviornment variable to null
	else if(position == 2){
	  setenv(args[1],"",1);
	}
	//if 2 arguments are found, sets the the first argument enviornment variable to the second argument
	else if(position == 3){
	  setenv(args[1],args[2],1);
	}
	// if more than 2 arguments are given to setenv, prints an error.
	else {
	  perror("setenv: Too many arguments.\n");
	}
      }/* end of setenv*/

					/* end of built-in commands*/    
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//     
  					/*  else  program to exec */

	/* external commands, fork(), exec()*/
      else
	{
	  //handleing all cases when an exec file is references
	  if(command[0]=='/'||command[0]=='.'){
	    int result;         
	    const char *filename = args[0];	
	    
	    result = access (filename, (X_OK && F_OK));
	    
	    if(result == -1){				\
	      perror("File Cannot be Accessed");
	    }
	    if(result == 0){
	      
	      pid_t  pid2;
	      int    status2;
	      //creates a child when exec is used
	      if ((pid2 = fork()) < 0) {    
		printf("ERROR: forking child process failed\n");
		exit(1);
	      }
	      //child sets a timer to 10 seconds
	      else if (pid2 == 0) {
		alarm(10); //timer
		signal(SIGKILL, kill_child); //kills child if timer runs out
		printf("Executing %s\n", args[0]);
		//executes child process
		if (execve(filename, args, envp) < 0) {    
		  printf("ERROR: exec failed \n");
		  exit(1);
		}
	      }
	      else {                                 
		while (wait(&status2) != pid2);
	      }    
	    }
	  }
	  
	  else{
	    pid_t  pid;
	    int    status;
	    
	    if ((pid = fork()) < 0) {    
	      printf("ERROR: forking child process failed\n");
	      exit(1);
	    }
	    /*Wild card implementation is here */
	    else if (pid == 0) {
	      //Checks too see if wild card is present
	      if(wildcard == TRUE && position > 1){
		printf("Executing %s\n", args[0]);
		//glob() searches for all pathnames matching the argument
		glob(args[1], GLOB_DOOFFS, NULL, &globbuf);
		if(position == 3){
		  glob(args[2], GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf);
		  globbuf.gl_pathv[1] = args[1];
		}
		globbuf.gl_pathv[0] = args[0];
		execvp(args[0], &globbuf.gl_pathv[0]);
	      }
	       else if (execvp(*argv, argv) < 0) {
		 printf("ERROR: exec failed command not found\n");
		 exit(1);
	       }
	    }
	    else {                                 
	      while (wait(&status) != pid);
	    }   
	  }	
	  
      }  
      
    }
  return 0;
}/* end of sh() */
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/* which function*/
char *which(char *command, struct pathelement *pathlist )
{
  /* loop through pathlist until finding command and return it.  Return
     NULL when not found. */
  struct pathelement *curr = pathlist;
  char* path;
  while(curr->next != NULL){
    //construct the path string here
    path = malloc(strlen(curr->element)+strlen(command)+2);
    strcat(path,curr->element);
    strcat(path,"/");
    strcat(path,command);
    if(access(path,F_OK)!=-1) {
      printf("%s\n", path);
      return path;
    }
    curr=curr->next;
  }
  printf("%s: Command not found.\n", command);  
}//* End of which()*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/* where funciton*/
char *where(char *command, struct pathelement *pathlist )
{
 /* similarly loop through finding all locations of command */
  // return;
  struct pathelement *curr = pathlist;
  char* path;
  int count = 0;
  char *returnval = " ";
  while(curr->next != NULL){
    //construct the path string here
    path = malloc(strlen(curr->element)+strlen(command)+2);
    strcat(path,curr->element);
    strcat(path,"/");
    strcat(path,command);
    printf("%s\n", path);
    curr=curr->next;
  }
  return path;
}/* end of where() */
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/* list function*/
void list ( char *dir )
{
  DIR             *dp;
  struct dirent   *dirp;
  //opes directory
  dp = opendir(dir);
  while ((dirp = readdir(dp)) != NULL){
    printf("%s\t", dirp->d_name);
  }
  printf("\n");
}/* end of list() */
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/* read line, reads the command line*/
char *read_line(void)
{
  char *line;
  ssize_t bufsize = 0;
  //ctrl D 
  while(!getline(&line, &bufsize, stdin)){
  }
  return line;
} /* end of read line */
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/* split_line, pareses command line input*/
char **split_line(char *line)
{
  //relevant variables
  int bufsize = BUFSIZE;                                // local buffersize
  int position = 0;                                     // sets position to 0
  char **tokens = malloc(bufsize * sizeof(char*));      // allocates memory to char** tokens
  char *token;						// variable that will store the seperate words

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  //uses DELIM to seperate the command line to seperate works
  token = strtok(line, DELIM);
  while (token != NULL) {
    //stores each word typed into command line into tokens
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, DELIM);
  }
  tokens[position] = NULL;
  //returns all seperated words in the command line which is stored in tokens*
  return tokens;
}/* end of split_line*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/* NODE struct, used for any singley linked list implemented
   takes a command and makes it into a node for traversal later*/
struct Node* GetNewNode(char* command) {
  struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
  newNode->command = malloc(sizeof(command)+1);
  strcpy(newNode->command, command);
  newNode->next = NULL;
  return newNode;
}/* end of Node*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/* PlaceInBack, places the command (node) onto the back of the linked list (of other commands)*/
void PlaceInBack(char* command){
  struct Node* temp = head;
  struct Node* newNode = GetNewNode(command);
  //if linked list is empty, make this node the head
  if(head == NULL) {
    head = newNode;
    return;
  }
  // loop until end of linked list, then add newNode at end  
  while (temp->next != NULL){
    temp = temp->next; 
  }
  temp->next = newNode;
}/* end of PlaceInback()*/
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/* CTRL Z and C handler, used to handle the signals from those two inputs.*/
void sigintHandler(int sig_num)
{
  printf("\n caught signal \n");
  fflush(stdout);
}/* end of Ctrl Z and C */
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//


//Extra Credit: Kills child if process runs out of time
void kill_child(int sig)
{
  printf("!!! taking too long to execute this command !!!");
  kill(pid, SIGKILL);
}/*end of kill_child*/

//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
//                                        /*END OF PROGRAM*/                                                //
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
