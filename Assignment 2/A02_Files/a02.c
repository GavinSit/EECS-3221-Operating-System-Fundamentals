#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>

#define POINTER_SPOTS 4 //how many threads can it hold

void logStart(char* tID);//function to log that a new thread is started
void logFinish(char* tID);//function to log that a thread has finished its time

void startClock();//function to start program clock
long getCurrentTime();//function to check current time since clock was started
time_t programClock;//the global timer/clock for the program

typedef struct thread //represents a single thread
{
	char tid[4]; //id of the thread as read from file
	long arrivalTime; //when the thread arrives to run
	long lifetime; //how long the thread runs for
	pthread_t threadID; //the id of the thread to be run
	_Bool threadStarted; //one process has started, dont run again
} Thread;

void* threadRun(void* t); //the thread function, the code executed by each thread
int readFile(char* fileName, Thread** threads); //function to read the file content and build array of threads

int main(int argc, char *argv[])
{
	if(argc<2)
	{
		printf("Input file name missing...exiting with error code -1\n");
		return -1;
	}

	Thread ** threads = (Thread **) malloc(sizeof(Thread *)); //allocate size for the pointer pointing to the thread pointer

	for (int i = 0; i < POINTER_SPOTS; i ++) { //allocate space for each index of the struct
		threads[i] = (Thread *) malloc (sizeof(Thread)); //for each index, allocate space for a thread
	}
	
	int numThreads = 0; //the total number of threads there are
	pthread_attr_t threadAttribute; //stores the attributes of the thread

	numThreads = readFile(argv[1], threads); //pass the file name as well as the Thread pointer that will store the threads struct
	
	pthread_attr_init(&threadAttribute); //get the default thread attributes passing the address for threadAttribute

	startClock(); //start the clock so the threads can be run
	
	int threadsRan = 0; //the number of threads that have run, used to keep track of when all threads are finished so program can exit

	//test print
	//for (int g = 0; g < numThreads; g++)
	//{
	//	printf("%s; %ld; %ld \n", threads[g]->tid, threads[g]->arrivalTime, threads[g]->lifetime);
	//}
	
	while(threadsRan < numThreads) //runs while there are still threads left to run
	{
		//starts each thread when it is time,
		for (int m = 0; m < numThreads; m++) //loop through the threads repeatedly
		{
			if (getCurrentTime() == threads[m]->arrivalTime && !threads[m]->threadStarted) //if time to run thread, run it
			{
				//create the thread that runs the process that just arrived
				threads[m]->threadStarted = 1; //set to 1 meaning the process has started and won't run again
				pthread_create(&threads[m]->threadID, &threadAttribute, threadRun, threads[m]); //create the thread and pass the Thread struct 

				threadsRan++; //after the thread runs, increase the counter storing number of run threads
			}
		} //end of for loop
	} //end of while

	//wait for each of the threads to finish before ending the program
	for (int n = 0; n < numThreads; n++)
	{
		pthread_join(threads[n]->threadID, NULL); //wait for the thread to finish
	}

	return 0;
} //end of main

int readFile(char* fileName, Thread** threads)//use this method in a suitable way to read file
{
	FILE *in = fopen(fileName, "r");
	if(!in) //no file there or incorrect file name passed
	{
		printf("Child A: Error in opening input file...exiting with error code -1\n");
		return -1;
	}
	
	
	struct stat st; //stores information about the file
	fstat(fileno(in), &st);  
	char* fileContent = (char*)malloc(((int)st.st_size+1)* sizeof(char)); //allocate size
	fileContent[0] = '\0'; //make empty

	//store each line of the file in a pointer array fileContent
	while(!feof(in)) //while not end of file
	{
		char line[100]; //temporarilty store the line from file before adding it to fileContent
		if(fgets(line,100,in)!=NULL)
		{
			strncat(fileContent,line,strlen(line)); //adds line to fileContent
		}
	}
	fclose(in); //close the file reader

	//count how many threads in total and allocate spacec for it
	char* command = NULL;
	int threadCount = 0; //how many threads in total
	char* fileCopy = (char*)malloc((strlen(fileContent)+1)*sizeof(char));
	strcpy(fileCopy,fileContent);
	command = strtok(fileCopy,"\r\n");
	while(command!=NULL)
	{
		threadCount++;
		command = strtok(NULL,"\r\n");
	}
	*threads = (Thread*) malloc(sizeof(Thread)*threadCount); //allocate space based on size of each thread as well as how many there are


	//gets the item on that line and stores it into lines array
	char* lines[threadCount];
	command = NULL;
	int i=0;
	command = strtok(fileContent,"\r\n");
	while(command!=NULL) //runs while there is a next line
	{
		lines[i] = malloc(sizeof(command)*sizeof(char)); //allocate space for each command that that is taken from the file and store it in lines
		strcpy(lines[i],command); //add to the lines
		i++; //move counter to next item to add the next lines if there is
		command = strtok(NULL,"\r\n"); //gets the next line from text file if there is
	}

	//takes each line and stores it in a thread 
	for(int k=0; k<threadCount; k++)
	{
		char* token = NULL;
		int j = 0;
		token =  strtok(lines[k],";"); //stores the items in that line in token
		/*
		1. thread_id
		2. start_time
		3. lifetime
		*/

		while(token!=NULL) //adds the items read to the thread pointer
		{
			switch (j)
			{
			case 0: //store the thread id
				strncpy(threads[k]->tid, token, 4);
				break;
			case 1: //store the start time of thread
				threads[k]->arrivalTime = atol(token);
				break;
			case 2: //store the lifetime of thread
				threads[k]->lifetime = atol(token);
				break;
			default:
				printf("Error! invalid case");
			}
			
			j ++;			
			token = strtok(NULL, ";");
		}
		threads[k]->threadStarted = 0; //the process has not been started so false

		//test print items stored
		//printf("%s %i %i \n", threads[k]->tid, threads[k]->arrivalTime, threads[k]->lifetime);
	}
	
	return threadCount;
}

void logStart(char* tID)//invoke this method when you start a thread
{
	printf("[%ld] New Thread with ID %s is started.\n", getCurrentTime(), tID);
}

void logFinish(char* tID)//invoke this method when a thread is over
{
	printf("[%ld] Thread with ID %s is finished.\n", getCurrentTime(), tID);
}


void* threadRun(void* t)//creates a thread to run for the specified amount of time
{
	Thread *thd = (Thread*) t; //cast void pointer t as Thread pointer pointing to the passed Thread struct
	logStart(thd->tid); //log the start time of this thread
	
	//while haven't reached completion yet, it'll keep looping till finished
	while (thd->arrivalTime + thd->lifetime != getCurrentTime()); //knowing the length of the thread as well as it starts running when it arrives, the finish time is start time + lifetime

	logFinish(thd->tid); //log the finish time of this thread

	pthread_exit(0); //exit the thread
}

void startClock()//invoke this method when you start servicing threads
{
	programClock = time(NULL); //set the time to be the beginning
}

long getCurrentTime()//invoke this method whenever you want to check how much time units passed
//since you invoked startClock()
{
	time_t now; //current time
	now = time(NULL);
	return now-programClock; //current time - start time = how much time has elapsed
}
