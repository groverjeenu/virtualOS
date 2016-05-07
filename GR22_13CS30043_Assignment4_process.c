// Assignment 4
// Virtual scheduler

// Objective
// In this assignment, we implement a virtual scheduler which runs on top of the existing linux scheduler
// and schedules one of the runnable processes according to the scheduling algorithm for execution.

// Group Details
// Member 1: Jeenu Grover (13CS30042)
// Member 2: Ashish Sharma (13CS30043)

// Filename: process.c


#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/msg.h>
#include <sys/ipc.h>

#define SCHEDULER_TYPE 1234

#define READY_TYPE 1
#define IO_TYPE 2
#define TERMINATE_TYPE 3
#define NEW_AND_READY_TYPE 4
#define MSGLENGTH 1

int SCHEDULER_KEY;

// Define the message struct
struct message
{
    long mtype;
    int pid;    // pid of the sender
    int info;   // NEW_AND_READY_TYPE or READY_TYPE
    int priority;   // Priority of the process
    char mtext[MSGLENGTH];  // Message to be sent, if any
};

size_t MSGLEN = sizeof(struct message) - sizeof(long) ;

// NOTIFY Signal handler (Resumes the process)
void Notify(int signum,siginfo_t * info, void * old)
{
    printf("Scheduler has sent signal for CPU Allocation\n");
    return;
}

// SUSPEND signal handler (Suspends the process and waits to be notified by the Scheduler)
void Suspend(int signum,siginfo_t * info, void * olde)
{

	sigset_t old,new;
	sigemptyset(&old);
	if(sigprocmask(SIG_UNBLOCK,NULL,&old)!=0)printf("Old signal set could not be retrieved\n");
	if(sigprocmask(SIG_UNBLOCK,&old,NULL)!=0)printf("Signals unblocked\n");
	printf("Received suspend and entered ready queue\n");
	pause();
	//printf("Pause ended\n");
	return;
}



int main(int argc , char *argv[])
{

    if(argc<6)
    {
        printf("Usage: %s NumberOfIterations Priority SleepProbability SleepDuration KEY\nTry Again...\n",argv[0]);
    }

	int NOI = atoi(argv[1]);    // Number Of Iterations
	int Priority = atoi(argv[2]);   // Priority
	double sleep_prob = atof(argv[3]);  // Sleep Probability
	int sleep_dur = atoi(argv[4]);  // Sleep Duration
	SCHEDULER_KEY = atoi(argv [5]); // KEY
	printf("\t\t%d\t\t\n",getpid());


	int mypid = getpid();


    // Install NOTIFY Signal Handler
	struct sigaction act1;
    act1.sa_flags = SA_SIGINFO;
    act1.sa_sigaction = &Notify;
	if (sigaction(SIGUSR1, &act1, NULL) == -1)
    {
        perror("sigusr1: sigaction");
        return 0;
    }

    // Install SUSPEND Signal Handler
    struct sigaction act2;
    act2.sa_flags = SA_SIGINFO;
    act2.sa_sigaction = &Suspend;
	if (sigaction(SIGUSR2, &act2, NULL) == -1)
    {
        perror("sigusr2: sigaction");
        return 0;
    }

    // GET THE Message Queue
	int msgid = msgget(SCHEDULER_KEY,IPC_CREAT|0666);
	if(msgid < 0)printf("Message Get Error\n");


	struct message msg,server_msg;
	msg.mtype = SCHEDULER_TYPE;
	msg.pid = mypid;
	msg.info = NEW_AND_READY_TYPE;
	msg.priority = Priority;
	//strcpy(msg.mtext,"Process created and signal sent to Scheduler\n");

    // Send READY Message to the scheduler
	if(msgsnd(msgid,&msg,MSGLEN,0) < 0 )printf("Failed to send creation message to scheduler\n"); //sends signal to schuder after creation
	else printf("Send creation message to scheduler\n");

    // Get Scheduler's PID
	if(msgrcv(msgid,&server_msg,MSGLEN,mypid,0) < 0 )printf("Failed to recieve message\n");
	else printf("Message conataining server pid received\n");
	int server_pid = server_msg.pid;


    // Wait for scheduler to NOTIFY
	printf("Waiting in Ready Queue\n");
	pause(); //Waits in ready queue
	printf("Started Execution\n");

	int looper = 0,rnum,low,high;
	low = (NOI - sleep_prob*NOI)/2;
	high = NOI -low;
	union sigval value;

	srand(time(NULL));

    // Start the Iterations
	for(looper=0;looper<NOI;looper++)
	{
	    // Check whether to sleep
		rnum = rand()%NOI;
		low = (NOI - sleep_prob*NOI)/2;
		high = NOI -low;
		printf("Iteration : %d\n",looper);

		if( rnum >= low && rnum < high)
		{
		    // Send I/O Request Signal
			value.sival_int = mypid;
			if(sigqueue(server_pid,SIGUSR1,value) != 0)printf("I/O request signal could not be sent\n");//Send notifaction for doing IO
			printf("Sent IO request\n");
			usleep(sleep_dur*50000); // Process doing I/O
			printf("IO completed\n");

            // Send I/O Complete Message
			msg.mtype = SCHEDULER_TYPE;
			msg.pid = mypid;
			msg.info = READY_TYPE;
			msg.priority = Priority;
			strcpy(msg.mtext,"Process created and signal sent to Scheduler\n");
			//printf("Going to send messgae\n");

			if(msgsnd(msgid,&msg,MSGLEN,0) < 0 )printf("Failed to send creation message to scheduler\n"); //sends signal to schuder after creation
			else printf("Send ready message to scheduler\n");

            // Wait for scheduler to NOTIFY
			printf("Waiting in Ready Queue\n");
			pause(); //Waits in ready queue
			printf("Started Execution\n");

		}

		/*
		if(looper == low)
		{
			value.sival_int = mypid;
			if(sigqueue(server_pid,SIGUSR1,value) != 0)printf("I/O request signal could not be sent\n");//Send notifaction for doing IO
			printf("Sent IO request\n");
			sleep(sleep_dur);
		}
		if(looper ==high)
		{
			printf("IO completed\n");
			msg.mtype = SCHEDULER_TYPE;
			msg.pid = mypid;
			msg.info = READY_TYPE;
			msg.priority = Priority;
			//strcpy(msg.mtext,"Process created and signal sent to Scheduler\n");
			printf("Going to send messgae\n");

			if(msgsnd(msgid,&msg,MSGLEN,0) < 0 )printf("Failed to send creation message to scheduler\n"); //sends signal to schuder after creation
			else printf("Send ready message to scheduler\n");

			printf("Waiting in Ready Queue\n");
			pause(); //Waits in ready queue
			printf("Started Execution\n");
		}
		if(looper > low && looper<high)
		{
			//sleep(sleep_dur); // Process doing I/O
		}
		*/
	}

    // If looped NOI times, send TERMINATE signal and finally terminate
	value.sival_int = mypid;
	if(sigqueue(server_pid,SIGUSR2,value) != 0)printf("Termination signal could not be sent\n");//Send notifaction for doing IO
	else printf("Terminating Process\n");

	return 0;
}
