
#include "get_path.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>


struct Node  {
  char *command;
  struct Node* next;
};


struct watchmailNode{
  char* file;
  pthread_t thread;
  struct watchmailNode* next;
};

struct watchuserNode{
  char* username;
  pthread_t thread;
  struct watchuserNode* next;
};



typedef struct watchuserNode watchuserList;
typedef struct watchmailNode watchmailList;
watchmailList* watchMailList;
watchuserList* watchUserList;
int watchmail(char* file, int off);
void* watchmail_thread(void*);
int watchuser(char* username, int disable);
void* watchuser_thread(void*);
void* warnload_thread(void*);

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
char **parseCmd(char* cmd);
void execute_cmd(char** args, int background, char** childpids, int nChildren);
int parse_redirection(char** command, char** file, char* line);
void perform_redirection(int* fid, char* redirect_file, int redirection_type);
void reset_redirection(int* fid,int redirection_type);
void parse_pipeline(char** left, char** right, char* line);
int get_load(double *loads);
void clean_up_child_process (int signal_number);

#define PROMPTMAX 32
#define MAXARGS 10

