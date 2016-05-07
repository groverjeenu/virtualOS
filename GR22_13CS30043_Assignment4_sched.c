// Assignment 4
// Virtual scheduler

// Objective
// In this assignment, we implement a virtual scheduler which runs on top of the existing linux scheduler
// and schedules one of the runnable processes according to the scheduling algorithm for execution.

// Group Details
// Member 1: Jeenu Grover (13CS30042)
// Member 2: Ashish Sharma (13CS30043)

// Filename: sched.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <langinfo.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/msg.h>


#define SCHEDULER_TYPE 1234

#define READY_TYPE 1
#define IO_TYPE 2
#define TERMINATE_TYPE 3
#define NEW_AND_READY_TYPE 4
#define MSGLENGTH 1



#define MAX 100000


// Struct for computing Waiting Time, Response Time and TurnAround Time
typedef struct time_proc{
    int pid;
    int responded; // 0 --- not responded, 1 ---- responded
    unsigned long first_ready_queue;
    unsigned long first_response;
    unsigned long term_time;
    unsigned long last_suspended;
    int waiting_time;
}TimeProc;

int SCHEDULER_KEY;

// Ready Queue Struct
typedef struct ReadyQueue{
    int pid;
    int priority;
}ReadyQueue;

ReadyQueue ready_queue[MAX];
TimeProc timeProc[MAX];
int front, rear,n = 0,f = 0;

int io_handled = 0;
int term_handled = 0;
int term_count = 0;

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



/****************************** Queue Methods **************************/

// Create Queue
void create_queue()
{
    front = rear = -1;
}

// Insert By Priority
void insert_by_priority(int p,int pid)
{
    int i,j;

    for (i = 0; i <= rear; i++)
    {
        if(pid == ready_queue[i].pid)
        {
           // printf("Process %d is already in the ready queue\n",pid);
            return;
        }
        else if (p < ready_queue[i].priority)
        {
            for (j = rear + 1; j > i; j--)
            {
                ready_queue[j].priority = ready_queue[j - 1].priority;
                ready_queue[j].pid = ready_queue[j - 1].pid;
            }
            ready_queue[i].pid = pid;
            ready_queue[i].priority = p;
            return;
        }
    }
    ready_queue[i].pid = pid;
    ready_queue[i].priority = p;
}

// Normal Insertion
void insert_normal(int p,int pid)
{
    int i;

    for (i = 0; i <= rear; i++)
    {
        if(pid == ready_queue[i].pid)
        {
           // printf("Process %d is already in the ready queue\n",pid);
            return;
        }
    }
    rear++;
    ready_queue[rear].pid = pid;
    ready_queue[rear].priority = p;
}


// Enqueue in the Ready Queue
void enqueue(int p,int pid,int schedule_type)
{
    if (rear >= MAX - 1)
    {
        //printf("Ready Queue is Full..Try Again Later\n");
        return;
    }
    if ((front == -1) && (rear == -1))
    {
        front++;
        rear++;
        ready_queue[rear].pid = pid;
        ready_queue[rear].priority = p;
        return;
    }
    else{
    	if(schedule_type == 0) {
            insert_by_priority(p,pid);
            rear++;
        }
    	else insert_normal(p,pid);
    }
}

// Delete From the Ready Queue
void delete_by_priority(int pid)
{
    int i;

    if ((front==-1) && (rear==-1))
    {
        //printf("Ready Queue is empty...No Processes to remove\n");
        return;
    }

    for (i = 0; i <= rear; i++)
    {
        if (pid == ready_queue[i].pid)
        {
            for (; i < rear; i++)
            {
                ready_queue[i].pid = ready_queue[i + 1].pid;
                ready_queue[i].priority = ready_queue[i + 1].priority;
            }

        ready_queue[i].pid = -100;
        ready_queue[i].priority = -100;
        rear--;

        if (rear == -1)
            front = -1;
        return;
        }
    }
    //printf("Process %d not found in the ready queue...Can't Delete\n", pid);
}

// Display the Ready Queue
void display_ready_queue()
{
    int i ;
    if ((front == -1) && (rear == -1))
    {
        printf("Ready Queue: empty\n");
        return;
    }

    printf("Ready Queue: ");

    for (i = front; i< rear; i++)
    {
        printf("(%d,%d), ", ready_queue[i].pid,ready_queue[i].priority);
    }
    printf("(%d,%d)\n", ready_queue[i].pid,ready_queue[i].priority);
}

// Check if the Ready Queue is empty
int is_empty()
{
    if ((front == -1) && (rear == -1))
    {
        return 1;
    }
    return 0;
}

// Dequeue from the Ready Queue
void dequeue()
{
    delete_by_priority(ready_queue[0].pid);
}

/****************************** End Of Queue Methods ******************************/

// Get the Current Time in microseconds
unsigned long getCurrTime()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
    return time_in_micros;
}

// Add a completely new Process in the TimeProc Struct and Initialize the parameters
void add_in_time(int pid)
{
    timeProc[n].pid = pid;
    timeProc[n].first_ready_queue = getCurrTime();
    timeProc[n].responded = 0;
    timeProc[n].waiting_time = 0;
    timeProc[n].last_suspended = getCurrTime();
    n++;
}


// Search the process pid in the TimeProc struct and update the respective times
void search_in_time(int pid, int type)
{
    int i;
    for(i=0;i<n;++i)
    {
        if(timeProc[i].pid == pid){
            if(type == 1){
                if(timeProc[i].responded == 0){
                    timeProc[i].responded = 1;
                    timeProc[i].first_response = getCurrTime();
                }
                return;
            }

            if(type == 2)
            {
                timeProc[i].term_time = getCurrTime();
                return;
            }

            if(type == 3)
            {
                timeProc[i].last_suspended = getCurrTime();
                return;
            }

            if(type == 4)
            {
                timeProc[i].waiting_time = timeProc[i].waiting_time+(getCurrTime() - timeProc[i].last_suspended);
                return;
            }

        }
    }
}


// I/O Request Handler
void IO_Request(int signum,siginfo_t * info, void * old)
{
    f = 1;
    io_handled = 1;
    int idx = info->si_value.sival_int;
    printf("Process %d requests I/O\n",idx);
}

// Term Request Handler
void Term_Request(int signum,siginfo_t * info, void * old)
{
    f = 1;
    int i;
    term_handled = 1;
    int idx = info->si_value.sival_int;
    search_in_time(idx,2);
    printf("Terminated: %d\n",idx);
    term_count++;

    // If all the processes have terminated, compute final results and store them in a file
    if(term_count == 4)
    {
        double avg_waiting_time = 0,avg_turnaround_time = 0, avg_response_time = 0;
        FILE *fp;
        fp = fopen("result.txt","w");
        fprintf(fp,"All time are in microseconds\n\n");
        for(i=0;i<n;++i)
        {

            fprintf(fp,"Process %d:\nResponse Time: %lu\nTurnAround Time: %lu\nWaitingTime: %d\n\n",timeProc[i].pid,(timeProc[i].first_response-timeProc[i].first_ready_queue),(timeProc[i].term_time-timeProc[i].first_ready_queue),timeProc[i].waiting_time);

            avg_waiting_time += timeProc[i].waiting_time;
            avg_response_time += (timeProc[i].first_response-timeProc[i].first_ready_queue);
            avg_turnaround_time += (timeProc[i].term_time-timeProc[i].first_ready_queue);
        }

        avg_waiting_time = (1.0*avg_waiting_time)/n;
        avg_turnaround_time = (1.0*avg_turnaround_time)/n;
        avg_response_time = (1.0*avg_response_time)/n;

        fprintf(fp, "Average Waiting Time: %lf \nAverage TurnAround Time: %lf \nAverage Response Time: %lf \n",avg_waiting_time,avg_turnaround_time,avg_response_time);
        fclose(fp);
    }
}


int main(int argc,char *argv[])
{
    int n,i;
    union sigval value;

    int schedule_pid = 0;
    int schedule_priority = 0;
    int schedule_type;

    create_queue();

    if(argc<4)
    {
    	printf("Usage: %s SchedulingType TIME_QUANTA KEY\nTry Again...\n",argv[0]);
    	return 0;
    }

    if(strcmp(argv[1],"PR") == 0){
    	schedule_type = 0;
    }

    else if(strcmp(argv[1],"RR") == 0)
    {
    	schedule_type = 1;
    }

    //printf("Schedule_Type: %d\n",schedule_type);

    int TIME_QUANTA = atoi(argv[2]);

    //TIME_QUANTA *= 50;

    SCHEDULER_KEY = atoi(argv [3]);

    struct message mymsg,sentMsg;

    // Create the Message Queue
	int msgid = msgget(SCHEDULER_KEY,IPC_CREAT|0666);
    if(msgid  < 0)printf("msgget Error\n");

    struct msqid_ds qstat;

    qstat.msg_qbytes = 800000;
    if(msgctl(msgid,IPC_SET,&qstat)<0)printf("Initialisation error\n");

    // Wait for a new process to be ready
    printf("Waiting For New Process...\n");
    if(msgrcv(msgid,&mymsg,MSGLEN,SCHEDULER_TYPE,0) < 0 ) printf("msrcv Error\n");

    // send own's PID to the New Process for further communications
    sentMsg.mtype = mymsg.pid;
    sentMsg.pid = getpid();

    if(msgsnd(msgid,&sentMsg,MSGLEN,0) < 0 )printf("msgsnd Error\n");

    else printf("PID Sucessfully sent\n");

    // insert the Process in the Ready Queue
    enqueue(mymsg.priority,mymsg.pid,schedule_type);

    // Add in TimeProc struct
    add_in_time(mymsg.pid);
    //search_in_time(schedule_pid,3);

   // printf("Process %d added to Ready Queue\n",mymsg.pid);
    //display_ready_queue();

    // Install I/O Request Handler
    struct sigaction act1;
    act1.sa_flags = SA_SIGINFO;
    act1.sa_sigaction = &IO_Request;

    // Install Termination Request Handler
    struct sigaction act2;
    act2.sa_flags = SA_SIGINFO;
    act2.sa_sigaction = &Term_Request;

    sigaction(SIGUSR1, &act1, NULL);
    sigaction(SIGUSR2, &act2, NULL);

    while (1)
    {
        // If Ready Queue is empty, wait for new processes
        if(is_empty()){
            printf("Waiting for receiving new process\n");
            if(msgrcv(msgid,&mymsg,MSGLEN,SCHEDULER_TYPE,0) < 0 ) printf("msrcv Error\n");

            if(mymsg.info == NEW_AND_READY_TYPE)
            {
                enqueue(mymsg.priority,mymsg.pid,schedule_type);
                add_in_time(mymsg.pid);
                //search_in_time(schedule_pid,3);

                //printf("Process %d added to Ready Queue\n",mymsg.pid);
                //display_ready_queue();

                sentMsg.mtype = mymsg.pid;
                sentMsg.pid = getpid();
                if(msgsnd(msgid,&sentMsg,MSGLEN,0) < 0 )printf("msgsnd Error\n");
            }
            else if(mymsg.info == READY_TYPE)
            {
                enqueue(mymsg.priority,mymsg.pid,schedule_type);

                search_in_time(mymsg.pid,3);

                printf("Process %d completes I/O\n",mymsg.pid);

            }

        }

        if(f == 1) f =0;

        // else, schedule a process from the ready queue
        schedule_pid = ready_queue[front].pid;
        schedule_priority = ready_queue[front].priority;
        // NOTIFY the Process to start it's execution
        if(sigqueue(schedule_pid,SIGUSR1,value) != 0) printf("NOTIFY signal could not be sent\n");

        // Update Times
        search_in_time(schedule_pid,1);
        search_in_time(schedule_pid,4);


        //else printf("NOTIFY: %d--------",schedule_pid);

        // Delete from the Ready Queue
        delete_by_priority(schedule_pid);

        printf("Process %d is running\n",schedule_pid);

        // Run the Process for the prescribed TIME_QUANTA
        for(i=0;i<TIME_QUANTA;++i)
        {
            usleep(100);

            // Terminate the loop in case I/O Request
            if(io_handled == 1)
            {
                io_handled = 0;
                f = 1;
                break;
            }
            // Terminate the loop in case Termination Request
            if(term_handled == 1)
            {
                term_handled = 0;
                f = 1;
                break;
            }

            // receive a new process parallely
            if(msgrcv(msgid,&mymsg,MSGLEN,SCHEDULER_TYPE,IPC_NOWAIT) >=0)
            {
                if(mymsg.info == NEW_AND_READY_TYPE)
                {
                	enqueue(mymsg.priority,mymsg.pid,schedule_type);
                    add_in_time(mymsg.pid);
                   // search_in_time(schedule_pid,3);

                   // printf("Process %d added to Ready Queue\n",mymsg.pid);
                    //display_ready_queue();

                    sentMsg.mtype = mymsg.pid;
                    sentMsg.pid = getpid();
                    if(msgsnd(msgid,&sentMsg,MSGLEN,0) < 0 )printf("msgsnd Error\n");
                }
                else if(mymsg.info == READY_TYPE)
                {
                	enqueue(mymsg.priority,mymsg.pid,schedule_type);

                    search_in_time(mymsg.pid,3);

                    printf("Process %d completes I/O\n",mymsg.pid);

    				//printf("Process %d added to Ready Queue\n",mymsg.pid);

                }
            }


            if(io_handled == 1)
            {
                io_handled = 0;
                f = 1;
                break;
            }

            if(term_handled == 1)
            {
                term_handled = 0;
                f = 1;
                break;
            }
        }

        if(io_handled == 1)
        {
            io_handled = 0;
            f = 1;
        }

        if(term_handled == 1)
        {
            term_handled = 0;
            f = 1;
        }

       // printf("Last Cpu Burst: %d\n",i);

        // If no I/O request or no Termination Request Suspend the Process
        if(f==0){
            if(sigqueue(schedule_pid,SIGUSR2,value) != 0) printf("SUSPEND signal could not be sent\n");
            //printf("-------------------------Suspended: %d----------------------\n",schedule_pid);
            // Enqueue it in the ready queue
            enqueue(schedule_priority,schedule_pid,schedule_type);
            // Update Times
            search_in_time(schedule_pid,3);
        }
        //display_ready_queue();
    }
    //printf("No Process in the Ready Queue...Quiting\n");
    return 0;
}
