/**
 * @file websh.c
 * @author Maurizio Rinder
 * @date 04.05.2012
 *
 * @brief Main program module.
 * 
 * This program takes Linux commands and puts the output to
 * html-tags.
 **/
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
#include<errno.h>

#define TAGSZ (256)
#define LINESZ (1025)
#define OPTIONS (3)
/****************************
Typedefs and structs
****************************/
/* enumeration for bool-flags - makes code more readable */
typedef enum{FALSE, TRUE} bool;
/* option structure that keeps all arguments given to the program */
struct opts{
  char *word;
  char *tag;
  bool optflags[OPTIONS];
};
/* a buffer structure for lines */
typedef struct{
  unsigned int linc;
  char **lines;
} lin_t;

/****************************
Global Variables
****************************/
/* Name of the program */
static const char *prgname = "websh";
/* Pipe file descriptors */
static int pipefd[2]; 
static int pipefd2[2];
/* all options of the program */
static struct opts *options;

static pid_t pid1;
/****************************
Function prototypes
*****************************/
/**
 * @brief checks programm call and given arguments
 * @param argc argument counter
 * @param argv options given by the user for the program
 * @param options all options that the program can get are stored in this variable
 */
static void parse_args(int argc, char **argv, struct opts *options);
/**
 * @brief adds line to the line collector struct
 * @param lcoll the line collector struct which gets the argument
 * @param arg argument to be added
 */
static void add_lin(lin_t *lcoll, const char *lin);
/**
* @brief destroys line collector
* @param lcoll the line collector struct to be destroyed
*/
static void destroy_lcoll(lin_t *lcoll);
/**
 * @brief surrounds a given string with a given tag
 * @param lin the line that gets surrounding
 * @param tag the tag that is used for surrounding
 * @param closeonly only adds a closing tag without opening tag
*/
static char *surround_line(const char *lin, const char *tag, const bool closeonly);
/**
 * @brief executes a given command
 * @detail this function writes the commands output to a pipe 
 * @param command the command line that has to be executed
 * @param istr_args a buffer for the demanded command options
 */
static void command_executer(const char *command, lin_t *istr_args);
/**
 * @brief reads the commands output and formats it to html-tags
 */
static void command_reader(void);
/***************************
Error routines
****************************/
/**
 * @brief prints usage of program
 */
static void usage(void);
/**
 * @brief terminates the program on program error
 * @param errnumber which is given at program exit
 * @param errmessage An errormessage that says what went wrong
 */
static void bailout(int errnumber, const char *errmessage);
/**
 * @brief prints error messages
 * @param errmessage
 */
static void printerror(const char *errmessage);
/**
 * @brief frees all used ressources
 */
static void free_ressources(void);
/***********************************
Main
***********************************/
int main(int argc, char **argv){
  /*======Variables===============*/
  lin_t *outln =0; 
  lin_t *istr_args =0;
  /*reading and output line*/
  char in_line[LINESZ], *out_line =0;
  /*helping variable for tokenizing (strtok) */
  char *pch=0;
  /* process identification */
  pid_t  pid2;
  /* several integer variables for counting and checking */
  int res =0, i =0;

  /*=====Argument check==========*/
 
  options = (struct opts *)malloc(sizeof(struct opts));
  for(i = 0; i < OPTIONS; ++i){ options->optflags[i] = FALSE; }
  parse_args(argc, argv, options);

  /************************/

  /**start the program**/
  outln = (lin_t *)malloc(sizeof(lin_t));
  outln->linc=0;
  istr_args = (lin_t *)malloc(sizeof(lin_t));
  istr_args->linc=0;

  while( (fgets(&in_line[0], LINESZ, stdin))!=NULL){
    /* ===Option -h handling=== */ 
    if( options->optflags[1] == TRUE ){
      pch = strtok(in_line,"\n");
      out_line = surround_line(pch, "h1", FALSE);
      add_lin(outln, out_line);
      free(out_line);
    }

    /* ===Pipe build up=== */
    res = pipe(pipefd);
    if( res < 0 ){
      bailout(EXIT_FAILURE, "main, Problem with pipes");
    }

    pid1 = fork();
    switch( pid1 ){
    case -1: 
      bailout(EXIT_FAILURE, "main, Problem with fork"); break;
    case 0:
      /*== child 1==*/
      command_executer(&in_line[0], istr_args); 
      break;
    default:
      res = pipe(pipefd2);
      if( res < 0 ){
	bailout(EXIT_FAILURE, "main, Problem with pipes");
      }
    }

    pid2 = fork();
    /**child process 2 **/
    switch( pid2 ){
    case -1:
      bailout(EXIT_FAILURE, "main, Problem with fork"); 
      break;
    case 0:
      command_reader();
      exit(EXIT_SUCCESS);
      break;
    default:
      if(close(pipefd[1]) < 0 || close(pipefd[0]) < 0 ){
	bailout(EXIT_FAILURE, "main, Problem with closing pipe");	
      }

      if(close(pipefd2[1]) < 0){
	bailout(EXIT_FAILURE, "main, Problem with closing pipe");       
      } 

      out_line = (char *)malloc(sizeof(char)*(LINESZ+2*TAGSZ));	    
      while( (read(pipefd2[0], out_line, (LINESZ+2*TAGSZ))) != 0 ){
	add_lin(outln, out_line);
      } 

      free(out_line);         

      waitpid(pid1, &res, WUNTRACED|WCONTINUED);
      waitpid(pid2, &res, WUNTRACED|WCONTINUED);
    } 
  }

  if( options->optflags[0] == TRUE ){
    (void)printf("<HTML><HEAD></HEAD><BODY>\n");
  }
  
  for(i = 0; i < outln->linc; ++i ){
    printf("%s\n", outln->lines[i]);
  }

  if( options->optflags[0] == TRUE ){ 
    (void)printf("</BODY></HTML>\n");
  }

  return EXIT_SUCCESS;
}

/***********************************
error functions
************************************/
static void usage(void){
  fprintf(stderr, "Usage: %s [-e] [-h] [-s WORD:TAG]\n", prgname);
  free_ressources();
  exit(EXIT_FAILURE);
}

static void bailout(int errnumber, const char *errmessage){
  if(errmessage != (const char *) 0 ){ printerror(errmessage); }
  free_ressources();
  exit(errnumber);
}
static void printerror(const char *errmessage){
  if( errno != 0 ){
    (void) fprintf(stderr, "%s: %s - %s\n", prgname, errmessage, strerror(errno));
  }else{
    (void) fprintf(stderr, "%s: %s\n", prgname, errmessage);
  } 
}
static void free_ressources(void){

}
/***********************************
functions
************************************/
static void parse_args(int argc, char **argv, struct opts *options){
  char c; char *pch; char *opt;

  while( (c = getopt(argc, argv, "ehs:")) != -1 ){
    switch( c ){
    case 'e': options->optflags[0] = TRUE; break;
    case 'h': options->optflags[1] = TRUE; break;
    case 's': options->optflags[2] = TRUE; opt = optarg; break; 
    case '?': usage(); break;
    default: assert(0);
    }
  }

  if( options->optflags[2] == TRUE ){
    pch = strtok(opt,":");
 
    if( pch == NULL ){ usage(); }
    options->word = strdup(pch);
    pch = strtok(NULL, ":");
    if( pch == NULL ){ usage(); }
    options->tag = strdup(pch);
    if( (strtok(NULL, ":")) != NULL ){
      usage();
    }
  }

}
static char *surround_line(const char *lin, const char *tag, const bool closeonly){
  char otag[TAGSZ], ctag[TAGSZ];
  char *resline =0;

  resline = (char *)malloc(sizeof(char) * (LINESZ+2*TAGSZ)); 
  /*=== opening tag ===*/
  if(closeonly == FALSE){
    strcpy(otag,"<");
    strcat(otag, tag);
    strcat(otag, ">");
    strcpy(resline, otag);
    strcat(resline, lin);
  }else{
    strcpy(resline, lin);
  }
   
  /*=== closing tag ===*/
  strcpy(ctag,"</");
  strcat(ctag, tag);
  strcat(ctag, ">");
  
  resline = strcat(resline, ctag);  
  return resline;
}

static void add_lin(lin_t *lcoll, const char *lin){
  int arrsz = lcoll->linc;

  lcoll->lines  = (char **)realloc((char**) lcoll->lines, sizeof(char *) * (arrsz+1));
  lcoll->lines[(arrsz)] = strdup(lin);
  lcoll->linc = (arrsz+1); 

}
static void destroy_lcoll(lin_t *lcoll){
 
  int arrsz = lcoll->linc;
  int i = 0;
  
  for(;i < arrsz; ++i){
    printf("killing line\n");
    free(lcoll->lines[i]);
  }

  free(lcoll);
  lcoll = (lin_t *)NULL;
  
}
static void command_executer(const char *command, lin_t *istr_args){
  char *pch =0, istr[LINESZ];
  int i =0;
 
  pch = strdup(command);
  pch = strtok(pch, " \n");
  if(pch != NULL){ 
    strcpy(istr, pch);
    add_lin(istr_args, pch); 
  }else{
    bailout(EXIT_FAILURE,"command_executer, No command given");
  }

  pch = strtok(NULL, " \n");
  while(pch != NULL){
    add_lin(istr_args, pch);
    pch = strtok(NULL, " \n");
  }

  if( close(pipefd[0]) < 0 ){
    destroy_lcoll(istr_args);
    bailout(EXIT_FAILURE, "main, Problem with pipe close");
  }

   if( (dup2(pipefd[1], 1)) < 0 ){
    destroy_lcoll(istr_args);
    bailout(EXIT_FAILURE, "main, Problem with dup2");
  }

  if( close(pipefd[1]) < 0 ){
    destroy_lcoll(istr_args);
    bailout(EXIT_FAILURE, "main, Problem with pipe close");
  }

  if( (execlp(istr, istr, NULL )) < 0 ){
    destroy_lcoll(istr_args);
    bailout(EXIT_FAILURE, "main, Problem with exec");
  }

}
static void command_reader(void){
  char in_line[LINESZ], *out_line =0, *pch =0;
  if( close(pipefd[1]) < 0 || close(pipefd2[0]) < 0){
    bailout(EXIT_FAILURE, "command_reader, Problem with closing pipe");
  }
         
  while(read(pipefd[0], &in_line[0], LINESZ) != 0 ){
    pch = strtok(&in_line[0], "\n");

    while(pch != NULL){
      if( options->optflags[2] == TRUE){
	if( (strstr(pch, options->word) ) != NULL ){
	  out_line = surround_line(pch, options->tag, FALSE);
	  out_line = surround_line(out_line, "br", TRUE);
	}
      }else{
	out_line = surround_line(pch, "br", TRUE);
      }     
 
      write(pipefd2[1], out_line, (LINESZ+TAGSZ*2)); 
      pch = strtok(NULL, "\n");
    }

  }

  if( close(pipefd[0]) < 0 || close(pipefd2[1]) < 0 ){
    bailout(EXIT_FAILURE, "command_reader, Problem with closing pipe");
  }

}
