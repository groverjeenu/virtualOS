// Assignment 4
// Virtual scheduler

// Objective
// In this assignment, we implement a virtual scheduler which runs on top of the existing linux scheduler
// and schedules one of the runnable processes according to the scheduling algorithm for execution.

// Group Details
// Member 1: Jeenu Grover (13CS30042)
// Member 2: Ashish Sharma (13CS30043)

// Filename: gen.c

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

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("Usage: %s KEY\nTry Again..\n",argv[0]);
        return 0;
    }

	int key = atoi(argv[1]);
	int N = 4,t = 1,i,pid;

    // Generate 2 CPU Bound Processes
	for(i = 0;i< N/2;i++)
	{
		pid = fork();
		if(pid == 0)
		{
			i = 2;

			execlp("xterm" ,"xterm" ,"-hold","-e","./process","10000","10","0.3","1",argv[1],(const char *)NULL);
			exit(1);
		}

		sleep(t);
	}

    // Generate 2 I/O Bound Processes
	for(i = 0;i< N/2;i++)
	{
		pid = fork();
		if(pid == 0)
		{
			i = 2;

			execlp("xterm" ,"xterm" , "-hold","-e","./process","4000","5","0.7","3",argv[1],(const char *)NULL);
			exit(1);
		}

		sleep(t);
	}

	return 0;
}
