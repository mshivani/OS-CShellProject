//By Shivani Murali and Lucas Grant

#include <sys/param.h>
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
#include <utmpx.h>
#include "sh.h"
#include <pthread.h>
#include <fcntl.h>
#ifdef HAVE_KSTAT
#include <kstat.h>
#endif

typedef int bool;
#define TRUE 1
#define FALSE 0

#define BUFSIZE 64
#define DELIM " \t\r\n\a"
int MAX_LENGTH = 256;
int HISTORY_MAX_SIZE;
struct Node *head, *tail;
char buf[512];    
const char* REDIRECT_OPERATORS[] = { ">>&", ">>", ">&", ">", "<" };      
int watchuserThread = 0;
int get_load(double *loads);
int noclobber = 0;
int warnloadThread = 0;
double loadValue;
int ret1;
sig_atomic_t child_exit_status;

int sh(int argc, char **argv, char **envp )
{  
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *prompt2 = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char **leftCommandTemp = calloc(MAX_CANON, sizeof(char*));
  char **rightCommandTemp = calloc(MAX_CANON, sizeof(char*));
  char **leftCommand = calloc(MAX_CANON, sizeof(char*));
  char **rightCommand = calloc(MAX_CANON, sizeof(char*));
  char **leftCommandRedirection = calloc(MAX_CANON, sizeof(char*));
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
  int background = 0;
  char **childpids = calloc(MAXARGS, sizeof(char*));
  int nChildren = 0;
  struct utmpx *up;
  int fid; 
  int directMethod;
  //int watchuserThread = 0;
  
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

      memset(rightCommand, 0, 255);
      memset(leftCommand, 0, 255);
      
      /* print your prompt */
      printf("%s",prompt);
      
      /* get command line and process */
      directMethod = -1;
      command = read_line();
      int c = 0;
      bool wildcard = FALSE;
      int pipeTrue = 0;
      
      while(command[c]!=NULL){
	if(command[c]== '*' || command[c]== '?'){
	  wildcard = TRUE;
	}
	/*		if(command[c] ==  '|') {
	 pipeTrue = 1;
	 }*/
	c++;
      }
          
      
      //check if redirection needs to happen, and if so redirect accordingly
      char* command_line = NULL;
      char* file = NULL;
      int whichRedirection = parse_redirection(&command_line, &file, 
					       command);
      
      if(whichRedirection >= 0){     
	if(whichRedirection == 0 ||  whichRedirection == 1 || whichRedirection == 4 || access(file, F_OK) == -1 ){
	  
	  strtok(file, "\n");
	  perform_redirection(&fid, file, whichRedirection); 	  
	  //  fprintf(stderr,"fid:%d \n", &fid);
        } 
	else {
	   if(noclobber == 1){
	      printf("Noclobber is on, Cannot overwrite existing file.\n");
	    }
	   else{
	     printf("File %s already exists. Do you want to overwrite it? (y/n) ", file);
	     char ans = getchar();
	     getchar();
	     if(ans == 'y' && noclobber == 0){
	       if(remove(file) == -1){
		 perror("Error in removing file");
	       }
	       strtok(file, "\n");
	       perform_redirection(&fid, file, whichRedirection);
	     } 
	   }
	
	}
	
      char* line_in_original = command;
      command = command_line;
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
     
     pipeTrue = 0;
     int pipeCount = 0;
     int whichPipe = 0;
     while(args[pipeCount]!=NULL){
       if(strcmp(args[pipeCount], "|") == 0) {
	 whichPipe = 1;
	 pipeTrue = 1;
	}
       if (strcmp(args[pipeCount], "|&") == 0){
	  whichPipe = 2;
	  pipeTrue = 1;
       }
       pipeCount++;
       }
     
     if(position>=1){
       if(strcmp(args[position-1], "&") == 0) {
	 // printf("& detected\n");
		background = 1;
		args[position-1] = '\0';
		position -= 1;
       }
       
	else{
	  background = 0;
	}
     }
     else{
       background = 0;
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


     //check if user is logged and notify if so
      else if (strcmp(command, "watchuser")==0 || strcmp(aliasCheck, "watchuser")==0){	
	char *username;
	username = (char*)malloc(strlen(args[1])+1);
	strcpy(username, args[1]);

	//not turned off
	 if(position == 2){
	  printf("Watching user for %s\n", username);
	  watchuser(username, 1);
	}
	else if((position == 3) && (strcmp(args[2], "off"))){
	  printf("Stopped watching user for %s\n", username);
	  watchuser(username, 0);
	}
      }

     //notify user when file size changes
      else if (strcmp(command, "watchmail")==0 || strcmp(aliasCheck, "watchmail")==0){
	char *file;
	file = (char*)malloc(strlen(args[1])+1);
	strcpy(file, args[1]);
	/*	if(position == 1){
	  printf("Too Few Arguments.\n");
	  }*/

	//not turned off
	 if(position == 2){
	  printf("Watching mail for %s\n", file);
	  watchmail(file, 1);
	}
	else if(position == 3){
	  printf("Stopped watching mail for %s\n", file);
	  watchmail(file, 0);
	}
	 /*	else if(position == 3){
	  printf("Not a valid argument\n");
	}
	else{
	  printf("Too many argumnets.\n");
	  }*/
	
      }


     //warnload if the systems load value is greater than the input value
else if (strcmp(command, "warnload")==0 || strcmp(aliasCheck, "warnload")==0){	
	if(position == 2){
	  loadValue = atof(args[1]);
	  float tempVal = 0;
	  //stop thread in val 0 but logic is in warnload_thread to exit
	  if(loadValue != tempVal){
	  printf("Watching loadValue %f\n", loadValue);
	  }
	  //if you dont want to watch load leve and set it to 0
	  else{
	     printf("Warning level of 0.0 detected, thread exited\n");
	  }
	  pthread_t loadThread;
	  //create thread once
	  if(warnloadThread == 0){
	    warnloadThread = 1;
	    pthread_create(&loadThread, NULL, warnload_thread, 
			   &loadValue);
	  }
	}
 }


//noclobber- changes global variable to handle file creatition differently
else if (strcmp(command, "noclobber")==0 || strcmp(aliasCheck, "noclobber")==0){
   if(noclobber == 0){
       printf("noclobber has been turned on\n");
     noclobber = 1;
   }
   else if(noclobber == 1){
     printf("noclobber has been turned off\n");
     noclobber = 0;
   }
 }	




      




					/* end of built-in commands*/    
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//   


  
      /*  else  program to exec */
      
 else
   {	
     //handles redirection case and executes command and uses fork, execvp, and waitpid in execute_cmd funtion
     	if(whichRedirection != -1){
	  execute_cmd(args, background, childpids, nChildren);
	  reset_redirection(&fid, whichRedirection);
	  }

//if pipleline is true executes right and left command

if(pipeTrue == 1){
	  
    int argCount = 0;
	  int leftCount = 0;
	  int rightCount = 0;
	  int detectPipeline = 0;
	  while(args[argCount]!=NULL){
	    if(detectPipeline == 1){
	      //right command to execute
	       rightCommand[rightCount] = args[argCount];
	     rightCount++;
	    }
	    if((strcmp(args[argCount], "|") == 0) || 
	       (strcmp(args[argCount], "|&") == 0)) {
	      detectPipeline = 1;
	    }
	    if(detectPipeline == 0){
	      //left command to execute
	       leftCommand[leftCount] = args[argCount];
	      leftCount++;
	     }
	    argCount++;
	    }
	 


	  int p[2];
	  int fid;

	  if(pipe(p) == -1){
	    perror("Error in pipline");
	  }

	  //redirect to stdin
	  close(0);
	  dup(p[0]);
	  close(p[0]);

	  //redirect to stdout
	  close(1);
	  dup(p[1]);

	  if(whichPipe == 2){
	    close(2);
	    dup(p[1]);
	  }

	  close(p[1]);

	  //execute command left of pipeline
	  execute_cmd(leftCommand, background, childpids, nChildren);
	  
	  fid =  open("/dev/tty", O_WRONLY);
	  close(1);
	  dup(fid);
	  close(fid);

	  fid = open("/dev/tty", O_WRONLY);
	  close(2);
	  dup(fid);
	  close(fid);

	  //execute command right of pipeline
	  execute_cmd(rightCommand, background, childpids, nChildren);
	  
	  fid = open("/dev/tty", O_RDONLY);
	  close(0);
	  dup(fid);
	  close(fid);
	  }
	  

	  

	else{
	  execute_cmd(args, background, childpids, nChildren);
	}


	     
      }
    }
return 0;
}/* sh() */

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
  char *line = calloc(PROMPTMAX, sizeof(char));
  int  bufsize = 100; 
  //ctrl D 
  while(!fgets(line, bufsize, stdin)){
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
/* watchmail checks to see if the file size has been changed and if so, displays a message that mail has been 
recieved */

int watchmail(char* file, int disable){
  if(disable == 1){
    // printf("create thread for watchMail\n");

    //add to list
    watchmailList* NewNode = (watchmailList*)malloc(sizeof(watchmailList));
    NewNode->file = file; 
    NewNode->next = watchMailList;
    watchMailList = NewNode;
    
 //create thread for each mail
    pthread_create(&(NewNode->thread), NULL, watchmail_thread, 
		   (void*)(NewNode->file));  
  } 
  else {        
    watchmailList* prev = NULL;
    watchmailList* curr = watchMailList;
    //iterate through list to find the file
    while(curr != NULL){
      if(!strcmp(curr->file, file)){
	break;
      }
      prev = curr;
      curr = curr->next;
    }    
    //thread does not exist for the file
    if(curr == NULL){
      fprintf(stderr, "watchmail thread for %s does not exist\n", file);
      return -1;
    }  
    pthread_cancel(curr->thread);
    
    // Delete the node from the watchmails linked list
    if(prev != NULL){
      prev->next = curr->next;
    } 
    else {
      watchMailList = curr->next;
    }
    free(curr->file);
    free(curr);
  }
  return 0;
}


//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/* watch user creates a list of users to be watched and compares it to the currently logged in list*/

int watchuser(char* username, int disable){
  if(disable == 1){
    // printf("create thread for watchUser\n");
    watchuserList* NewNode = (watchuserList*)malloc(sizeof(watchuserList));
    NewNode->username = username; 
    NewNode->next = watchUserList;
    watchUserList = NewNode;
    
    if(watchuserThread == 0){
       // printf("create thread for watchUser\n");

      //only create thread once but globally update list
      pthread_create(&(NewNode->thread), NULL, watchuser_thread, 
		   (void*)(NewNode->username));
      watchuserThread == 1;
    }
  } 
  else {        
    watchuserList* prev = NULL;
    watchuserList* curr = watchUserList;
    struct utmpx *up;

    //iterate through list to find the file
    while(curr != NULL){
      setutxent();
      while(up = getutxent()){
	if(up->ut_type == USER_PROCESS){
	  if(!strcmp(curr->username, up->ut_user)){
	    break;
	  }
	}
	
      }
      prev = curr;
      curr = curr->next;
    }    
    //if user is not in the list
    if(curr == NULL){
      fprintf(stderr, "The user %s is not being watched\n", username);
      return -1;
    }  
    
    
    // Delete the node from the watchuser linked list
    if(prev != NULL){
      printf("User %s has logged off\n", curr->username);
      prev->next = curr->next;
    } 
    else {
      printf("User %s has logged off\n", curr->username);
      watchUserList = curr->next;
    }
    free(curr->username);
    free(curr);
  }
  return 0;
}


//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/*Watchmail thread creates the initail thread and notifies user when mail has been recieved by comapring it
to the file size*/

void* watchmail_thread(void* param){

  // printf("inside watch mail thread\n");
    char* filename = (char*)param;
    struct stat stat_info;
    off_t last_size;        

    stat(filename, &stat_info);
    last_size = stat_info.st_size;

    while(1) {
        stat(filename, &stat_info);
	//compare file sizes
        if(stat_info.st_size > last_size){
	  struct timeval tp;
	  gettimeofday(&tp, NULL);
	  printf("\nBEEP You've Got Mail in %s at %s\n", filename, 
		 ctime(&(tp.tv_sec)));
        }
	
        last_size = stat_info.st_size;
        sleep(1);
    }
    return NULL;    
}

//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/*Warnload thread creates the initial thread and notifies user when CPU load level is > the watched load level*/
void* warnload_thread(void* param){

  float zeroVal = 0;
  double loads[3];
  while(1){
    if(loadValue == zeroVal){
      warnloadThread = 0;
      pthread_exit(NULL);
    }


//find out load level                               
#ifdef HAVE_KSTAT
  kstat_ctl_t *kc;
  kstat_t *ksp;
  kstat_named_t *kn;

  kc = kstat_open();
  if (kc == 0)
  {
    perror("kstat_open");
    exit(1);
  }
  ksp = kstat_lookup(kc, "unix", 0, "system_misc");
  if (ksp == 0)
  {
    perror("kstat_lookup");
    exit(1);
  }
  if (kstat_read(kc, ksp,0) == -1) 
  {
    perror("kstat_read");
    exit(1);
  }
  kn = kstat_data_lookup(ksp, "avenrun_1min");
  if (kn == 0) 
  {
    fprintf(stderr,"not found\n");
    exit(1);
  }
  loads[0] = kn->value.ul/(FSCALE/100);
  kn = kstat_data_lookup(ksp, "avenrun_5min");
  if (kn == 0)
  {
    fprintf(stderr,"not found\n");
    exit(1);
  }
  loads[1] = kn->value.ul/(FSCALE/100);
  kn = kstat_data_lookup(ksp, "avenrun_15min");
  if (kn == 0) 
  {
    fprintf(stderr,"not found\n");
    exit(1);
  }
  loads[2] = kn->value.ul/(FSCALE/100);
  kstat_close(kc);

#endif

      if(loads[0]/100 > loadValue){
	  printf("WARNING: current load level is %f, and warnlevel is %f\n",loads[0]/100,loadValue);
      }
      sleep(30);

  }
}



//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/*Watchuser thread creates the initail thread and notifies user when a watched user is logged on*/

void* watchuser_thread(void* param){
 // printf("inside watch user thread\n");
    char* username = (char*)param;

    watchuserList* prev = NULL;
    watchuserList* curr = watchUserList;
    struct utmpx *up;
    
    while(1) {
      while(curr != NULL){
	setutxent();
	//logic to determine if any user from the watched user list is in the currently logged in list
	while(up = getutxent()){
	  if(up->ut_type == USER_PROCESS){
	    if(!strcmp(curr->username, up->ut_user)){
	      printf("%s has logged on %s from %s\n", up->ut_user, up->ut_line, up ->ut_host);	 
	      if(prev != NULL){
		prev->next = curr->next;
	      } 
	      else {
		watchUserList = curr->next;
	      }
	      free(curr->username);
	      free(curr);
    
	      break;
	    }
	  }
	  
	}
	prev = curr;
	curr = curr->next;
      } 	
        sleep(60);
    }
    return NULL; 

}


//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/*Parse the command line is redirection and also return an int value that determines which redirection it is*/

int parse_redirection(char** command, char** file, char* line){  

    char* rd_stdout = NULL;
    int redirect_code;

    int i =0;
    while((i < 5) && (rd_stdout == NULL)){
        rd_stdout = strstr(line, REDIRECT_OPERATORS[i]);
        redirect_code = i;
	i++;
    }

    if(rd_stdout == NULL){
        return -1;
    }
    //alloacte memory and parse to set file and command
    int command_length = (int)rd_stdout - (int)line;
    *command = (char*)malloc(command_length + 1);
    memcpy(*command, line, command_length);
    (*command)[command_length - 1] = '\0';
    int file_length = strlen(line) - (int)rd_stdout + (int)line;
    char* ptr = strtok(line + command_length, " >&<");
    *file = (char*)malloc(file_length); 
    memcpy(*file, ptr, strlen(ptr) + 1);
    //returns which type of redirection
    return redirect_code;
}



//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/*Redirectes to file by seting read, wrie, and appending*/

void perform_redirection(int* fid, char* redirect_file, int whichRedirection){
    int open_flags = O_CREAT;
   
    //switches between cases for each redirection and assigns flags
    switch(whichRedirection){
        case 0:
        case 1:
        case 2:
        case 3:
            open_flags |= O_WRONLY;
            break;
        case 4:
            open_flags |= O_RDONLY; 
            break;
    }

    switch(whichRedirection){
        case 0:
        case 1:
            open_flags |= O_APPEND;
            break;
        case 2:
        case 3:
        case 4:
            break;
        default:
            break;
    }

     *fid = open(redirect_file, open_flags, 0666);

     //performs the redirection
    switch(whichRedirection){
        case 0:
        case 2:
            close(2);
            dup(*fid);
        case 1:
        case 3:
            close(1);
            dup(*fid);
            close(*fid);
            break;
        case 4:
            close(0);
            dup(*fid);
            close(*fid);
            break;
        default:
            break;
    }
}


//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/*Logic to redirect back to screen*/
void reset_redirection(int* fid, int redirection_type){
    if(redirection_type != -1 && redirection_type != 4){
        *fid = open("/dev/tty", O_WRONLY);
        close(2);
        dup(*fid);
        close(*fid);
        *fid = open("/dev/tty", O_WRONLY);
        close(1);
        dup(*fid);
        close(*fid);
    } 
    else if(redirection_type == 4){
        *fid = open("/dev/tty", O_RDONLY);
        close(0);
        dup(*fid);
        close(*fid);
    }
}


//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/*handles fork, execute, and background process*/

void execute_cmd(char** args, int background, char** childpids, int nChildren){

  pid_t child_pid;

  /* Handle sigchild using clean_up_child_process function */
  struct sigaction sigchld_action;
  memset (&sigchld_action, 0, sizeof (sigchld_action));
  sigchld_action.sa_handler = &clean_up_child_process;
  sigaction (SIGCHLD, &sigchld_action, NULL);
  
  /* Create child*/
  child_pid = fork ();
  if (child_pid > 0) {
    
    /* Parent process */
    if (background == 0) {
      waitpid(child_pid, NULL, WNOHANG);
    } 
    
    else { 
      printf("starting background job %d\n", child_pid);
      childpids [nChildren] = child_pid;
      nChildren++;
      sleep (60);
    }  
  }
  else { 
    if(background == 1) {
      printf("Child background process...");
      
      fclose(stdin); // close child's stdin
      fopen("/dev/null", "r"); // open a new stdin that is always empty
      execvp(*args,args);
      
      // If an error occurs, print error and exit
      fprintf (stderr, "unknown command: %s\n", args[0]);
      exit(1);   
    } 
    else {
      execvp(*args,args);
      
      // If an error occurs, print error and exit
      fprintf (stderr, "unknown command: %s\n", args[0]);
      exit(1);
    }
  }	
}



void clean_up_child_process (int signal_number)
{
  /* Clean up the child process. */
  int status;
    wait (&status);
 
  /* Store its exit status in a global variable. */
  child_exit_status = status;
}



//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
/*gets current load of system*/

int get_load(double *loads)
{
#ifdef HAVE_KSTAT
  kstat_ctl_t *kc;
  kstat_t *ksp;
  kstat_named_t *kn;

  kc = kstat_open();
  if (kc == 0)
  {
    perror("kstat_open");
    exit(1);
  }

  ksp = kstat_lookup(kc, "unix", 0, "system_misc");
  if (ksp == 0)
  {
    perror("kstat_lookup");
    exit(1);
  }
  if (kstat_read(kc, ksp,0) == -1) 
  {
    perror("kstat_read");
    exit(1);
  }

  kn = kstat_data_lookup(ksp, "avenrun_1min");
  if (kn == 0) 
  {
    fprintf(stderr,"not found\n");
    exit(1);
  }
  loads[0] = kn->value.ul/(FSCALE/100);

  kn = kstat_data_lookup(ksp, "avenrun_5min");
  if (kn == 0)
  {
    fprintf(stderr,"not found\n");
    exit(1);
  }
  loads[1] = kn->value.ul/(FSCALE/100);

  kn = kstat_data_lookup(ksp, "avenrun_15min");
  if (kn == 0) 
  {
    fprintf(stderr,"not found\n");
    exit(1);
  }
  loads[2] = kn->value.ul/(FSCALE/100);

  kstat_close(kc);
  return 0;
#else
  /* yes, this isn't right */
  loads[0] = loads[1] = loads[2] = 0;
  return -1;
#endif
} /* get_load() */





//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//
//                                        /*END OF PROGRAM*/                                                //
//----------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------//

