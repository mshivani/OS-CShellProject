
#include "get_path.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

struct Node  {
  char *command;
  struct Node* next;
};

int pid;
void kill_child(int sig);
int sh( int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
void list ( char *dir );
void printenv(char **envp);
char *read_line(void);
char **split_line(char *line);
struct Node* GetNewNode(char* command);
void PlaceInBack(char* command);
void PrintForward();
void sigintHandler(int sig_num);


#define PROMPTMAX 32
#define MAXARGS 10
