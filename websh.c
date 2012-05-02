/**
Purpose:


 */
/******************************
Includes and definitions
******************************/
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>

#define LINESZ (1025)
/****************************
Typedefs
****************************/
typedef enum{FALSE, TRUE} bool;
/****************************
Global Variables
****************************/
static const char *prgname = "websh";
static FILE *src_file;
/****************************
Function prototypes
*****************************/

/***************************
Error routines
****************************/
static void usage(void);

int main(int argc, char **argv){
  /**Variables**/
  char *opt=0, *word=0, *tag=0, *pch=0;
  char *in_line =0;
  char *cmd[1];

  char *command=0;
  char c;

  pid_t pid;
  int pipefd[2];

  int res;
  bool optflags[] = {FALSE, FALSE, FALSE};
  /*optflags:
    optflags[0] ... -e
    optflags[1] ... -h
    optflags[2] ... -s
  */

  /**Option check **/
  while( (c = getopt(argc, argv, "ehs:")) != -1 ){
    switch( c ){
    case 'e': optflags[0] = TRUE; break;
    case 'h': optflags[1] = TRUE; break;
    case 's': optflags[3] = TRUE; opt = optarg; break; 
    case '?': usage(); break;
    default: assert(0);
    }
  }

  if( optflags[3] == TRUE ){
    pch = strtok(opt,":");
    word = pch;
    pch = strtok(NULL, ":");
    tag = pch;

    if( (strtok(NULL, ":")) != NULL ){
      usage();
    }
  }

   printf("Optioncheck succeeded\n"); 
   /**start the program**/

   src_file = stdin;
   pch =0;
   res = pipe(pipefd);
   if( res < 0 ){
     /*error output*/
   }
   in_line = (char *)malloc(LINESZ * sizeof(char));
   while( (fgets(in_line, LINESZ, src_file))!=NULL){

     /*first process*/
     pid = fork();
     switch(pid){
     case -1: /*error output*/ break;
     case 0: /*child*/ 
       close(pipefd[0]);
       dup2(pipefd[1], 1);
       close(pipefd[1]);
       execlp("ls", "ls", "-l", NULL);
       exit(EXIT_SUCCESS);
       break;
     default: /*parent*/
       wait(&res);
       printf("child closed: %d\n", pid);
       break;
     }
     /*** doesnt work by now ***/
     /*second process*/
     pid = fork();
     switch(pid){
     case -1: /*error output*/ break;
     case 0: /*child*/
       close(pipefd[1]);
       /** that loop has a problem **/
       while (read(pipefd[0], in_line, LINESZ) != 0){
	 printf("got a line\n");
       }
       printf("exitting the child-programm\n");
       exit(EXIT_SUCCESS);
       break;
     default: /*parent*/
       wait(&res);
       printf("child closed: %d\n", pid);
       break;
     }
   }
  return EXIT_SUCCESS;
}

static void usage(void){
  fprintf(stderr, "Usage: %s [-e] [-h] [-s WORD:TAG]\n", prgname);
  exit(EXIT_FAILURE);
}
