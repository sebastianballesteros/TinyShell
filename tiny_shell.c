#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <sched.h> //clone
#include <time.h> //time
#include <fcntl.h> //pipe

char** parse_line(char *line){
  //delimiter to tokenize the line i.e boundaries
  char* delimiter = " \t\r\n";
  size_t buffer_size = 512; //buff size = 512b
  int index = 0;
  //this is gonna be the array of string that is gonna be returned
  char **args = malloc(buffer_size * sizeof(char*));
  char *token;

    //couldn't allocate the array, exit with error code 1
  if ( args == NULL) exit(EXIT_FAILURE);

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

int clone_function( void* arg){
    char** command  = (char**) arg;
  if (execvp(command[0], command) < 0) {
    printf("Command failed");
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
      exit(EXIT_FAILURE);
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

  if(pid < 0) exit(EXIT_FAILURE);

  //child
  if (pid == 0){
    if(execvp(command[0], command)  == -1) {
      printf("Call unsuccesfull\n");
      exit(EXIT_FAILURE);
    }
  }
}

void clone_defined(char** command){
      //defined to implement cd
      char  *gdir;
      char  *dir;
      char  *to;
      char buf[512];
      //cd case

       if (!strcmp(command[0], "cd")){

            gdir = getcwd(buf, sizeof(buf));
            dir = strcat(gdir, "/");
            to = strcat(dir, command[1]);
	    chdir(to);

       }
      char* stack = malloc(512);
      char* stack_top = stack + 512;
      int flags = CLONE_VFORK | CLONE_FS;
      int child_pid = clone(&clone_function, stack_top, flags | SIGCHLD , command);
      int status;
     wait(&status);
}

void fifo_defined(char** command){

    pid_t pid = fork();

    if (pid == 0) {
       close(1);
       int file = open("fifo", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
       dup(file); // remap output back to parent
       execlp(command[0], command[0], NULL);
       close(file);
    }else{
      int status;
      waitpid(pid, &status, 0);
      // remap output from previous child to input
      close(0);
      int file = open("fifo", O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR);

      if(command[1]!=NULL){
        execlp(command[1], command[1], NULL);
      }else{
	char* response = (char*) malloc(512);
	read(file, response, 512);
	printf( response);
      }
      close(file);
    }
}


int my_system( char* line){

  char ** command = parse_line(line);

  // Exit para salir del shell
  if (!strcmp(command[0], "exit")) exit(EXIT_SUCCESS);

  clock_t begin;
  clock_t end;
  double time_spent;
  FILE* file = fopen("time.txt", "a");

  //If fork is defined
  #ifdef FORK
  begin = clock();
  fork_defined(command);
  end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  fprintf (file, "Time spent with fork() in %s: %f\n",command[0], time_spent);
  return 0;

  //If Vfork is defined
  #elif defined(VFORK)
  begin = clock();
  vfork_defined(command);
  end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  fprintf (file, "Time spent with vfork() in %s: %f\n",command[0], time_spent);
  return 0;

  //if clone is defined
  #elif defined(CLONE)
  begin = clock();
  clone_defined(command);
  end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  fprintf (file, "Time spent with clone() in %s: %f\n",command[0], time_spent);
  return 0;


  #elif PIPE
  fifo_defined(command);
  return 0;

  #else
  printf("%s\n", "Please specify a compile flag");
  #endif

}

char* get_a_line(){
  int bytes_read;
  size_t buffer_size = 512; //buff size = 64 b
  char* buffer = (char *)malloc(buffer_size * sizeof(char));

  //couldn't allocate the buffer, exit with error code 1
  if( buffer == NULL) exit(1);

  //read line from stdin
  bytes_read =   getline(&buffer, &buffer_size, stdin);
  //if failed to read, exit with error code 2
  if(bytes_read == 0) exit(2);


  return buffer;
}

int length(char* line){
  int length = 0;
  //10 is the ascii for line feed
  for(int i=0; line[i] != '\0' && line[i] != 10 ; i++){
    length++;
  }
  return length;
}


int main(int argc, char *argv[]){
  char* line;
  int status;

  while (1) {
    //Just to make the shell look better
    system("pwd");
    printf("%s", " ~ " );
    line = get_a_line();
    if(length(line) > 1) {
      status = my_system(line);
    }else{
      return 0;
    }
  }
}
