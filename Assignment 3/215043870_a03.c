#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <semaphore.h>
#include <stdbool.h>

void logStart(char* tID);//function to log that a new thread is started
void logFinish(char* tID);//function to log that a thread has finished its time

void startClock();//function to start program clock
long getCurrentTime();//function to check current time since clock was started
time_t programClock;//the global timer/clock for the program

typedef struct thread //represents a single thread, you can add more members if required
{
	char tid[4];//id of the thread as read from file
	unsigned int startTime;
	int state; 
	/*
	state = 0 means hasnt arrived yet
	state = 1 means arrived and started
	state = -1 means finished thread
	*/
	pthread_t handle;
	int retVal; //return value of 0 if thread creation was successful
} Thread;

int threadEvenOdd(char *t); //check if thread is even or odd
void firstRun(char* t); //thread uses the even/odd semaphores a little different for the first thread that run
void onlyEvenOddLeft(Thread* threads, int tCount); //check if there is only even or only odd left

int threadsLeft(Thread* threads, int threadCount); //the number of threads left to run
int threadToStart(Thread* threads, int threadCount); //the thread to start running
void* threadRun(void* t);//the thread function, the code executed by each thread
int readFile(char* fileName, Thread** threads);//function to read the file content and build array of threads

//global variables
sem_t crit_section_sem; //semaphore object for the critical section
sem_t even; //semaphore specifying that it is time for even operation
sem_t odd; //semaphore specifying that it is time for odd operation
bool onlyOneKind = false; //changed to true when there is only one kind left (even/odd)

int main(int argc, char *argv[])
{
	if(argc<2)
	{
		printf("Input file name missing...exiting with error code -1\n");
		return -1;
	}

	sem_init(&crit_section_sem, 0, 1); //initalize the critical section semaphore
	sem_init(&even, 0, 1); //initalize the even semaphore
	sem_init(&odd, 0, 1); //initalize the odd semaphore

	Thread* threads = NULL;
	int threadCount = readFile(argv[1],&threads); //get the total number of threads there are

	startClock(); //start the clock to keep track of the thread

	while(threadsLeft(threads, threadCount)>0)//put a suitable condition here to run your program
	{
    //you can add some suitable code anywhere in this loop if required

		int i = 0;
		while((i=threadToStart(threads, threadCount))>-1)
		{
			onlyEvenOddLeft(threads, threadCount); //make it so the other items can run if there is only one kind of number left
			firstRun(threads[i].tid); //make changes to even/odd semaphore depending on whether first number is even or odd
			threads[i].state = 1;
			threads[i].retVal = pthread_create(&(threads[i].handle),NULL,threadRun,&threads[i]);
		}
	}
	
	//destroy the semaphores after finishing with it
	sem_destroy(&crit_section_sem); 
	sem_destroy(&even);
	sem_destroy(&odd);

	return 0;
}

int readFile(char* fileName, Thread** threads)//do not modify this method
{
	FILE *in = fopen(fileName, "r");
	if(!in)
	{
		printf("Child A: Error in opening input file...exiting with error code -1\n");
		return -1;
	}

	struct stat st;
	fstat(fileno(in), &st);
	char* fileContent = (char*)malloc(((int)st.st_size+1)* sizeof(char));
	fileContent[0]='\0';	
	while(!feof(in))
	{
		char line[100];
		if(fgets(line,100,in)!=NULL)
		{
			strncat(fileContent,line,strlen(line));
		}
	}
	fclose(in);

	char* command = NULL;
	int threadCount = 0;
	char* fileCopy = (char*)malloc((strlen(fileContent)+1)*sizeof(char));
	strcpy(fileCopy,fileContent);
	command = strtok(fileCopy,"\r\n");
	while(command!=NULL)
	{
		threadCount++;
		command = strtok(NULL,"\r\n");
	}
	*threads = (Thread*) malloc(sizeof(Thread)*threadCount);

	char* lines[threadCount];
	command = NULL;
	int i=0;
	command = strtok(fileContent,"\r\n");
	while(command!=NULL)
	{
		lines[i] = malloc(sizeof(command)*sizeof(char));
		strcpy(lines[i],command);
		i++;
		command = strtok(NULL,"\r\n");
	}

	for(int k=0; k<threadCount; k++)
	{
		char* token = NULL;
		int j = 0;
		token =  strtok(lines[k],";");
		while(token!=NULL)
		{
//if you have extended the Thread struct then here
//you can do initialization of those additional members
//or any other action on the Thread members
			(*threads)[k].state=0;
			if(j==0)
				strcpy((*threads)[k].tid,token);
			if(j==1)
				(*threads)[k].startTime=atoi(token);
			j++;
			token = strtok(NULL,";");
		}
	}
	return threadCount;
}

void logStart(char* tID)
{
	printf("[%ld] New Thread with ID %s is started.\n", getCurrentTime(), tID);
}

void logFinish(char* tID)
{
	printf("[%ld] Thread with ID %s is finished.\n", getCurrentTime(), tID);
}

int threadsLeft(Thread* threads, int threadCount)
{
	int remainingThreads = 0;
	for(int k=0; k<threadCount; k++)
	{
		if(threads[k].state>-1)
			remainingThreads++;
	}
	return remainingThreads;
}

int threadToStart(Thread* threads, int threadCount)
{
	for(int k=0; k<threadCount; k++)
	{
		if(threads[k].state==0 && threads[k].startTime==getCurrentTime())
			return k;
	}
	return -1;
}

void* threadRun(void* t)//implement this function in a suitable way
{
	//used to store the number from the thread id which will be checked if even/odd
	int evenValue, oddValue; //used to get semaphoe value to check if it is the first run
	sem_getvalue(&even, &evenValue);
	sem_getvalue(&odd, &oddValue);

	logStart(((Thread*)t)->tid); //log the start time of the thread 

	if (!onlyOneKind) //if there is more than one kind of thread left
	{ //dont need to alternate even/odd if only one kind left
		if (threadEvenOdd(((Thread*)t)->tid) == 0) { //if thread is even
		   //wait even and free odd 
			sem_wait(&even);
		}
		else {//if thread is odd
		   //wait odd and free even 
			sem_wait(&odd);
		}
	}

	sem_wait(&crit_section_sem);
	//critical section starts here
	printf("[%ld] Thread %s is in its critical section\n",getCurrentTime(), ((Thread*)t)->tid);
	//critical section ends here
	sem_post(&crit_section_sem);

	if (threadEvenOdd(((Thread*)t)->tid) == 0) { //if thread is even
		if (onlyOneKind)
			sem_post(&even);
		else 
			sem_post(&odd);
	}
	else {//if thread is odd
		if (onlyOneKind)
			sem_post(&odd);
		else
			sem_post(&even);
	}


	logFinish(((Thread*)t)->tid); //log the finish time of the thread
	((Thread*)t)->state = -1;
	pthread_exit(0);
}

void startClock()
{
	programClock = time(NULL);
}

long getCurrentTime()//invoke this method whenever you want check how much time units passed
//since you invoked startClock()
{
	time_t now;
	now = time(NULL);
	return now-programClock;
}

int threadEvenOdd(char * t) //check if thread is even or odd
{ //return 0 if even and 1 if odd
	//check character 2
	int num = t[2] - '0'; //store as int
	if (num % 2 == 0) {
		return 0;
	}else {
		return 1;
	}
}

void firstRun(char * t) //first run, check even or odd and trigger the corresponding semaphore
{
	static bool ranBefore = false;
	if (!ranBefore) //if hasnt run before
	{
		ranBefore = true;
		if (threadEvenOdd(t) == 0) { //first thread even
			sem_wait(&odd); //trigger odd so that running even will unlock odd for the next thread
		}
		else { //first thread odd
			sem_wait(&even); //trigger even so that running odd will unlock even for next thread
		}
	}
}

void onlyEvenOddLeft(Thread* threads, int tCount) //returns 0 if there is only 1 kind left, returns 1 if there is only odd or only even left
{ //will return true if there are no unrun statements
	bool hasEven = false;
	bool hasOdd = false;

	for (int i = 0; i < tCount; i++)
	{
		if (threads[i].state != -1) //if thread hasnt run and finished
		{
			if (threadEvenOdd(threads[i].tid) == 0) //if even
			{
				hasEven = true;
			}
			else  //if odd 
			{
				hasOdd = true;
			}
		}
	}

	if (!(hasEven & hasOdd)) //if dont have both even and odd
	{
		int evenValue, oddValue; //values of the semaphore
		sem_getvalue(&even, &evenValue);
		sem_getvalue(&odd, &oddValue);
		if (evenValue == 0) //release the even semaphore if needed
			sem_post(&even);

		if (oddValue == 0) //rellease the odd semaphore if needed
			sem_post(&odd);

		onlyOneKind = true; //used in the threads to check if there is only one kind left
	}
}