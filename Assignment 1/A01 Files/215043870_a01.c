/*
Name: Gavin Sit
Student Number: 215 043 870
Description: Assignment 1
Citation:
- used code in textbook on pages 133, 134, 141, 142 
- https://www.geeksforgeeks.org/creating-multiple-process-using-fork/
	-> used as a guide on how to create more than one child process to do different things
- https://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/fork/create.html
- https://linuxhint.com/exec_linux_system_call_c/
	-> used to learn about the different exec
*/

#include<stdio.h> //printing and file operations
#include<unistd.h> // fork, ftruncate, and exec
#include <stdlib.h> //malloc, atoi
#include<string.h> //strcat
#include <sys/wait.h> //wait
#include <sys/mman.h> //mmap
#include <fcntl.h> //the O constants
#include <sys/types.h> //data types like pid_t

#define PIPE_SIZE 1000 //size of the pipe
#define READ_PIPE 0 //constant for read
#define WRITE_PIPE 1 //constant for write

//function declaration
void writeOuput(char* command, char* output); //prints command and result of command

int main(int argc, char *argv[]) {
	char ** commandArray; //used later by parent to store all commands retrieved from shared memory 
	int index = 0; //used to traverse the array as well as as a counter
	
	pid_t pid; //pid_t is the data type that stands for process ids 
	
	pid = fork(); //create child process
	/*
	neg - child creation failed
	0 - returned to newly created child
	pos - returned to parent (value is the child process)
	*/
	
	//shared memory stuff
	const int SIZE = 4096; //size of shared memory object in bytes
	const char *sharedName = "OS"; //name of memory object
	//const char *msg;
	int fd; //file descriptor
	char *memptr; //pointer to shared memory
	
	int lines = 0; //how many commands there are
	
	if (pid<0){ //error occured
		fprintf(stderr, "Fork Failed\n");
		return 1;
	
	} else if (pid == 0){ //child process 1 to get file
		FILE *fptr = fopen (argv[1], "r"); 
		char content [15]; //content of file to read to be returned
		char *str; //stores all the lines	
		
		/*
		read file and return content of file through a shared memory as string
		*/
		if (fptr == NULL) {
			printf("Error, invalid file! \n");
		} else { //file exists, so do something
			str = (char*) malloc(200*sizeof(char)); //allocate space for 200 characters
			
			//store each line of the file in an array 
			while(fgets (content, 15, fptr)) {
				strcat(str, content);
				lines ++; //increment the total number of lines there are in the program
			}
		}
		sprintf(content, "\n%d", lines);//change int to string
		strcat(str, content);
		fclose(fptr); //close the file pointer		
		
		//child process returns message through pipe
		fd = shm_open (sharedName, O_CREAT | O_RDWR, 0666); //create shared memory
		/* 
		O_CREAT - creates shared memory object if it doesnt existy
		O_RDWR - open object for read/write access
		0666 - directory priviledges
		*/
		
		ftruncate(fd, SIZE); //set size of shared object
		
		//memory map the shared memory
		memptr = (char *) mmap (0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		/*
		PROT_READ - can read
		PROT_WRITE - can write
		MAP_SHARED - other processes can see updates made to the same area
		*/
		
		//add msg to memptr
		sprintf(memptr, "%s", str); //msg is pointed at by memptr

		return 0; //end here because finished
	} 
	
	wait(NULL); //waits until child process is finished
		
	//parent accesses message through shared memory
	fd  = shm_open(sharedName, O_RDWR, 0666);
	/*
	O_RDONLY - read only access
	*/
	
	//memory map the shared memory
	memptr = (char *) mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	//store extracted stuff in array
	char *extractedString; //the string extracted
	commandArray =  malloc(sizeof(char *) * lines);
	
	extractedString = strtok((char *)memptr, "\r\n"); 
	
	while (extractedString != NULL) { //while there are more lines in the string to read
		commandArray[index] = malloc (sizeof(char *) * strlen(extractedString)+1); //allocate the appropriate size
		commandArray[index] = extractedString;
		index ++; 
		
		extractedString = strtok(NULL, "\r\n"); //needs to be null to continue
	}
	lines = atoi(commandArray[index-1]); //the last line is the total number of commands we have
	shm_unlink(sharedName); //unlink from the shared memory object
	
	// Pass the commands from command array to be executed by child
	//char write_message[PIPE_SIZE]; //message to pass
	char read_message[PIPE_SIZE]; //message to read
	int pipefd[2]; //used for pipe
	int i;
	char command[30]; //stores an unmodified version of the command as strtok modifies it
	
	//loop through all the commands, execute them using child and return with pipe
	//parent will print and continue until all commands are executed
	for (i = 0; i < lines; i ++) { 
		//create pipe
		if (pipe(pipefd) == -1) { //error creating pipe
			fprintf(stderr, "Pipe Failed\n");
			return 1;
		}
		
		strcpy(command, commandArray[i]); //copy an unmodified version of command
		
		pid = fork();
		
		if (pid < 0){ //error occured
			fprintf(stderr, "Fork Failed\n");
			return 1;
			
		} else if (pid == 0){ //child process 2 executes the command
			//execute one command at a time and return everything
			dup2(pipefd[WRITE_PIPE], STDOUT_FILENO); //redirect the output to pipe which will be read by parent

			close(pipefd[READ_PIPE]); //close the read end of the pipe
			close(pipefd[WRITE_PIPE]); //close the write end of the pipe
			
			//seperate the lines by the spaces
			char *args[20];
			int g = 0; //index of args
			
			args[g] = strtok(commandArray[i], " ");
			
			while (args[g] != NULL) { //while there are more things to read		
				g++; 
				args[g] = strtok(NULL, " "); //needs to be null to continue
			}
			g++;
			args[g] = NULL; //add the null at the end 
			
			execvp(args[0], args); //run the command and the output is captured using dup2
			
			return 0;	
			
		} else { //parent
			wait(NULL); //wait until child 2 is complete
			close(pipefd[WRITE_PIPE]); //close the write end of the pipe
			
			read(pipefd[READ_PIPE], read_message, PIPE_SIZE); //read the result of output
			writeOuput(command, read_message); //print the result
			close(pipefd[READ_PIPE]); //close the read end of the pipe
			memset(read_message, 0, sizeof(read_message)); //reset the variable
			memset(command, 0, sizeof(command)); //reset the variable
		}	
	}

	return 0;
}

void writeOuput(char* command, char* output) {
	printf("The output of: %s : is\n", command);
	printf(">>>>>>>>>>>>>>>\n%s<<<<<<<<<<<<<<<\n", output);	
}