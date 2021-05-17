#include<stdio.h> //printing and file operations
#include<unistd.h> // fork, ftruncate, and exec
#include <stdlib.h> //malloc, atoi
#include<string.h> //strcat
#include <sys/wait.h> //wait
#include <sys/mman.h> //mmap
#include <fcntl.h> //the O constants
#include <sys/types.h> //data types like pid_t


#define die(e) do { fprintf(stderr, "%s\n", e); exit(EXIT_FAILURE); } while (0);

int main() {
  int link[2];
  pid_t pid;
  char foo[4096];

  if (pipe(link)==-1)
    die("pipe");

  if ((pid = fork()) == -1)
    die("fork");

  if(pid == 0) {
		
    dup2 (link[1], STDOUT_FILENO);
    close(link[0]);
    close(link[1]);
    //execl("/bin/ls", "ls", "-1", (char *)0);
		
		char *args[] = {"ls", "-l", "-a", "-F", NULL};
    //char *string; = "ls -l -a -F"; //the string extracted
		//char *extractedString;
		//char *args [10];

			

		 execvp (args[0], args);
		die("execl");

  } else {

    close(link[1]);
    int nbytes = read(link[0], foo, sizeof(foo));
    printf("Output: (%.*s)\n", nbytes, foo);
    wait(NULL);

  }
  return 0;
}