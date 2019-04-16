#define _GNU_SOURCE
#include <sched.h> //clone
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <wordexp.h>
#include <ctype.h>
#include <sys/stat.h>
#define _VFORK

char** parse_line(char *line){
  //delimiter to tokenize the line i.e boundaries
  char* delimiter = " \t\r\n";
  size_t buffer_size = 512; //buff size = 64 b
  int index = 0;
  //this is gonna be the array of string that is gonna be returned
  char **args = malloc(buffer_size * sizeof(char*));
  char *token;

    //couldn't allocate the array, exit with error code 1
  if ( args == NULL) exit(3);

  //strtok returns a pointer to the first token in the string
  token = strtok(line, delimiter);

  //so while we have tokens in the string
  while (token != NULL) {
    //Fill the array
    args[index] = token;
    index++;
    //go to the next token
    token = strtok(NULL, delimiter);
  }

  //put a NULL at the end of the array cause execvp need it
  args[index] = NULL;
  return args;
}

int child_function( char** arg){
  char** command = (char**) arg; 
  if (execvp(arg[0], arg) < 0) {
    printf("Failed");
  }
 return 0;
}

void fork_defined(char** command){
  int pid = fork();

  //needed to send it to the waitpid function
  int status;

  if(pid < 0) exit(4);
  //child
  if (pid == 0){
    if(execvp(command[0], command)  == -1) {
      printf("Call unsuccesfull\n");
      exit(5);
    }
  }
  else{
    do{
      //WUNTRACED is --> Report status of stopped child process.
      waitpid(pid, &status, WUNTRACED);
    } while(!WIFEXITED(status) && !WIFSIGNALED(status));
  }

}

void vfork_defined(char** command){
  int pid = vfork();
  clock_t begin = clock();

  if(pid < 0) exit(4);

  //child
  if (pid == 0){
    if(execvp(command[0], command)  == -1) {
      printf("Call unsuccesfull\n");
      exit(5);
    }
  }
}

void clone_defined(char** command){
    const int DEFAULT_STK_SZ = 65536;
  	char* stack = malloc(DEFAULT_STK_SZ);
  	char* stack_top = stack + 65536;
    int flags = CLONE_VFORK | CLONE_FS;
  	int child_pid = clone(&child_function, stack_top, flags | SIGCHLD , command);
  	int status;
  	wait(&status);
}

void fifo_defined(char** command){
   int file_descriptor;
   char* filename = "output.txt";
   int fifo[2];
   close(1);

   if((file_descriptor = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR)) == -1){
       exit(2);
   }

   if(pipe(fifo) == -1){
       exit(2);
   }


  int pid = fork();
  //needed to send it to the waitpid function
  int status;

  if(pid < 0) exit(4);
  //child
  if (pid == 0){

    if(execvp(command[0], command)  == -1 && strstr(command[0], "fifo") == NULL ) {
      printf("Call unsuccesfull\n");
      exit(5);
    }
  }
  else{
    do{
      //WUNTRACED is --> Report status of stopped child process.
      waitpid(pid, &status, WUNTRACED);
    } while(!WIFEXITED(status) && !WIFSIGNALED(status));
  }
}

int my_system( char* line){

  char ** command = parse_line(line);
  clock_t begin;
  clock_t end;
  double time_spent;
  if(strstr(line, "fifo") != NULL){
    begin = clock();
    fifo_defined(command);
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time Spent with fifo(): %f\n", time_spent);
    return 0;
  }

  //If fork is defined
  #ifdef _FORK
  begin = clock();
  fork_defined(command);
  end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("Time Spent with fork(): %f\n", time_spent);
  return 0;

  //If Vfork is defined
  #elif defined(_VFORK)
  begin = clock();
  vfork_defined(command);
  end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("Time Spent with vfork(): %f\n", time_spent);
  return 0;

  //if clone is defined
  #elif defined(_CLONE)
  begin = clock();
  clone_defined(command);
  end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("Time Spent with clone(): %f\n", time_spent);
  return 0;


  #elif defined(_FIFO)
  begin = clock();
  fifo_defined(command);
  end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("Time Spent with fifo(): %f\n", time_spent);
  return 0;

  #else
  return 0;

  #endif

}
