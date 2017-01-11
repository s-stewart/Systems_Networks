/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 6
 *
 * This file contains the CPU scheduler for the simulation.  
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os-sim.h"


/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
int timeslice = -1;
int round_robin = 0;
int priority = 0;
static pthread_mutex_t current_mutex;
static pthread_mutex_t rqueue_mutex;
char empty = 1;
pthread_cond_t not_empty;


pcb_t *ready_queue_head = NULL;		/*ready queue initially empty*/
pcb_t *ready_queue_tail = NULL;



/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running process indexed by the cpu id. 
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    /* FIX ME */
	
	pcb_t* front;
	/*if head is the front of the queue, can just remove from head and set head pointer to the previous head's next*/
	
	pthread_mutex_lock(&rqueue_mutex);				/*lock the mutex*/
	front = ready_queue_head;

	if (front != NULL){						/*if the ready queue is not empty*/
		if(ready_queue_head == ready_queue_tail && ready_queue_head != NULL){
			ready_queue_head = NULL;
			ready_queue_tail = NULL;
		}
		else
			ready_queue_head = ready_queue_head->next;		/*remove from head*/

		front->state = PROCESS_RUNNING;
		if (round_robin == 1)
			context_switch(cpu_id,front,timeslice);
		else
			context_switch(cpu_id,front,-1);			/*schedule that process*/
		printf("Trying to schedule %s\n", front->name);
	}
	else
		context_switch(cpu_id,NULL,-1);

	
	if (ready_queue_head == NULL){
		empty = 1;
	}
	pthread_mutex_unlock(&rqueue_mutex);				/*unlock the ready queue*/
	
	pthread_mutex_lock(&current_mutex);				/*lock the current list*/
	current[cpu_id] = front;
	pthread_mutex_unlock(&current_mutex);

}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
** */
extern void idle(unsigned int cpu_id)
{
    /* FIX ME */
	pthread_mutex_lock(&rqueue_mutex);				/*lock the ready queue*/
	while(empty){							/*while the queue is empty*/
		pthread_cond_wait(&not_empty,&rqueue_mutex);		/*wait untill it is not*/
	}
	pthread_mutex_unlock(&rqueue_mutex);				/*unlock the ready queue*/
   	schedule(cpu_id);

    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
   	pthread_mutex_lock(&current_mutex);		/*need both mutexes*/
	pthread_mutex_lock(&rqueue_mutex);
	
	if (ready_queue_tail != NULL){
		ready_queue_tail->next = current[cpu_id];/*make the current end point to the process thats going to be the new end*/
		ready_queue_tail = current[cpu_id];	/*make the process the new tail*/
	}

	else{
		ready_queue_tail = current[cpu_id];
		ready_queue_head = current[cpu_id];
	}

	pthread_mutex_unlock(&current_mutex);
	pthread_mutex_unlock(&rqueue_mutex);

	schedule(cpu_id);

}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    /* FIX ME */
	pthread_mutex_lock(&current_mutex);			/*lock before modification*/
	current[cpu_id]->state = PROCESS_WAITING;
	pthread_mutex_unlock(&current_mutex);
	schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    /* FIX ME */
	pthread_mutex_lock(&current_mutex);			/*lock before modification*/
	current[cpu_id]->state = PROCESS_TERMINATED;
	pthread_mutex_unlock(&current_mutex);
	schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is static priority, wake_up() may need
 *      to preempt the CPU with the lowest priority process to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with a higher priority than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for 
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    /* FIX ME */
	
	/*1*/
	process->state = PROCESS_READY;
	pthread_mutex_lock(&rqueue_mutex);			/*lock the ready queue*/
	if (empty == 1){					/*if the ready queue is empty*/
		ready_queue_tail = process;			/*insert*/
		ready_queue_head = process;
		empty = 0;
		pthread_mutex_unlock(&rqueue_mutex);		/*unlock the ready queue*/
	}
	
	else {
		ready_queue_tail->next = process;
		ready_queue_tail = process;
		empty = 0;
		pthread_mutex_unlock(&rqueue_mutex);
	
	}
	
	pthread_cond_broadcast(&not_empty);			/*ready queue is not empty*/

	
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{
    int cpu_count;
    /* Parse command-line arguments */
    if (argc < 2)
    {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler\n"
            "         -p : Static Priority Scheduler\n\n");
        return -1;
    }
    cpu_count = atoi(argv[1]);


	
    /* FIX ME - Add support for -r and -p parameters*/
    if (argc > 2){
    	if (strcmp(argv[2], "-r") == 0){
			/*round robin*/
		timeslice = atoi(argv[3]);
		round_robin = 1;
        }

	/*else if (strcmp(argv[2], "-p") == 0){
		priority = 1;
	}*/

	else
		printf("Not a valid input option\n");
	
    }
	
	
 

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
	pthread_mutex_init(&rqueue_mutex, NULL);
	pthread_cond_init(&not_empty,NULL);
	
    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}


