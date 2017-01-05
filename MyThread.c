#include <malloc.h>
#include <ucontext.h>
#include <stdio.h>
#include "MyThread.h"
#include "MyHeader.h"

#define Stack_Size 1024*64
ucontext_t parent;
//ucontext_t* ready;
//ucontext_t* blocked;
int ready_start, ready_end, blocked_start, blocked_end, sizeq = 10;

queue* ready_q;
queue* blocked_q;
MyThreadInfo *running_t, *threadinit_t, *main_t;


void initializations()
{
	// point the head and tail of ready and blocked queues to NULL i.e initialize them	
	ready_q = (queue*) malloc(sizeof(queue));
	ready_q->head = NULL;
	ready_q->tail = NULL;
	blocked_q = (queue*) malloc(sizeof(queue));
	blocked_q->head = NULL;
	blocked_q->head = NULL;
	// initialize running thread
	running_t = (MyThreadInfo*) malloc(sizeof(MyThreadInfo));
	main_t = (MyThreadInfo*) malloc(sizeof(MyThreadInfo));
}


MyThreadInfo* set_to_next(queue *q)
{
	MyThreadInfo *temp = (MyThreadInfo*) malloc(sizeof(q_element));
	temp = q->head->thread;	
	if(q->tail == q->head) // if reached the end of queue, set headand tail to NULL
	{
		q->head = NULL;
		q->tail = NULL;
	}
	else
	{
	// point the head of ready_q to next element	
		q->head = q->head->next;
	}
	return temp;
}


void enqueue(queue *q, MyThreadInfo *t)
{
	// make q_element to be inserted into queue using ThreadInfo
	q_element *temp = (q_element*) malloc(sizeof(q_element));
	temp->thread = t;
	temp->next = NULL;

	//insert the q_element into tail of the queue
	if(q->head == NULL && q->tail == NULL) //if this is the first element in the queue
	{
		
		q->head = temp; //point head to 1st element
		q->tail = temp; //point tail to first element
		q->size++;
//		printf("qsize = %d\n", q->size);
	}
	else // if q_element is not the first element of the queue
	{
		q->tail->next = temp; //point current tail to temp
		q->tail = temp; // update tail
		q->size++;
	}
}


int element_presence(queue *q, MyThreadInfo *t)
{
	q_element *temp = (q_element*) malloc(sizeof(q_element));
	temp = q->head; // point to q_element at head of queue
	while(1)
	{
		if(temp->thread == t)// if MyThreadInfo found in queue
		{	
			return 0;
		}
		if(temp->thread == NULL) //If reached the end of queue and still not found element
		{
			return 1;
		}
		temp = temp->next;
	}
}


void MyThreadExit(void)
{	
	//save current context somewhere to use in swap
	MyThreadInfo *curr = running_t;
	if(running_t != main_t)
	running_t->parent->childrunning--;
	if(ready_q->size > 0)
	{	
		ready_q->size--;		
		if(running_t->joined == 1)
		{
			running_t->parent->childwaiting--;
			if(running_t->parent->childwaiting == 0)
			enqueue(ready_q, running_t->parent);
		}
		if(ready_q->size != 0)		
		running_t = set_to_next(ready_q);
		else
		running_t = main_t;
		if(running_t == main_t)
		{
			enqueue(ready_q,running_t);
			ready_q->size--;	
			running_t = set_to_next(ready_q);	
		}
		getcontext(&curr->context);
		swapcontext(&curr->context, &running_t->context);
	}
	else
	{
		running_t = main_t;
		swapcontext(&curr->context, &main_t->context);
	}
}


void MyThreadJoinAll()
{
	MyThreadInfo *curr = running_t;
	running_t->childwaiting = running_t->childrunning;	
	enqueue(blocked_q, running_t);
	ready_q->size--;
	running_t = set_to_next(ready_q);
	if(running_t == main_t)
	{
		enqueue(ready_q,running_t);
		ready_q->size--;		
		running_t = set_to_next(ready_q);	
	}
	if(curr->childrunning > 0)
	{
		swapcontext(&curr->context, &running_t->context);
	}
}


int MyThreadJoin(MyThread thread)
{
	MyThreadInfo *curr = running_t;
	MyThreadInfo *tojoin = thread;
	if (tojoin->parent != running_t)
	{
        	return -1;
    	}
	if(element_presence(ready_q, tojoin) != 0) // if the element is not present in the queue, then it has already terminated.
	{
		printf("Already exited\n");
		return 0; // Proceed with execution of program
	}
	else
	{
	// If the element is present in the queue
	//add the running_t to blocked queue
	running_t->childwaiting++;
	enqueue(blocked_q, running_t);
	ready_q->size--;
	tojoin->joined = 1;
	running_t = set_to_next(ready_q);
	if(running_t == main_t)
	{
		enqueue(ready_q,running_t);
		ready_q->size--;		
		running_t = set_to_next(ready_q);	
	}
	swapcontext(&curr->context, &running_t->context);
	return 0;
	}
}



void MyThreadYield(void)
{	
	//save current context somewhere to use in swap
	MyThreadInfo *curr = running_t;
	// add the thread that yielded to end of ready queue
	enqueue(ready_q, running_t);
	ready_q->size--;
	// set the new head to running thread and immediately setcontext
	running_t = set_to_next(ready_q);
	if(running_t == main_t)
	{
		enqueue(ready_q,running_t);
		ready_q->size--;
		running_t = set_to_next(ready_q);	
	}
	getcontext(&curr->context);
	swapcontext(&curr->context, &running_t->context);
}


MyThread MyThreadCreate(void(*start_funct)(void *), void *args)
{
	// make MyThreadInfo with function to be executed
	MyThreadInfo *child_t = (MyThreadInfo*) malloc(sizeof(MyThreadInfo));
	getcontext(&child_t->context);	
	child_t->context.uc_link = 0;
	child_t->context.uc_stack.ss_sp = malloc(Stack_Size);
	child_t->context.uc_stack.ss_size = Stack_Size;
	child_t->context.uc_stack.ss_flags = 0;
	child_t->parent = running_t;
	running_t->childrunning ++;
	makecontext(&child_t->context, (void(*) ())start_funct, 1, args);
	// add the child_t to ready queue
	enqueue(ready_q, child_t);
	return (MyThread) child_t;
}

void MyThreadInit(void(*start_funct)(void *), void *args)
{	
	initializations();
	// take context of parent (mythreadinit)
	getcontext(&parent);	
	parent.uc_link = 0;
	parent.uc_stack.ss_sp = malloc(Stack_Size);
	parent.uc_stack.ss_size = Stack_Size;
	parent.uc_stack.ss_flags = 0;
	
	//make a queue element with threadinfo of first function user wants to execute
	getcontext(&main_t->context);
	main_t->context.uc_link = 0;
	main_t->context.uc_stack.ss_sp = malloc(Stack_Size);
	main_t->context.uc_stack.ss_size = Stack_Size;
	main_t->context.uc_stack.ss_flags = 0;
	main_t->parent = NULL;
	main_t->child_q = (queue*) malloc(sizeof(queue));
	main_t->child_q->head = NULL;
	main_t->child_q->tail = NULL;
	makecontext(&main_t->context, (void(*) ())start_funct, 1, args);

	// set running thread to main_t and immediately do swapcontext
	enqueue(ready_q, main_t);	
	running_t = main_t;
	swapcontext(&parent, &running_t->context);
}






//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


MySemaphore MySemaphoreInit(int initialValue)
{
	semaphore *curr = (semaphore*)malloc(sizeof(semaphore));

	if(initialValue<0)
	{
		printf("Set the initial value to initialValue, which must be non-negative\n");
		return NULL;
	}

	curr->semaphore_q = (queue*) malloc(sizeof(queue));
	curr->semaphore_q->head = NULL;
	curr->semaphore_q->tail = NULL;
	curr->semaphore_q->size = 0;
	curr->initial_value = initialValue;
	curr->final_value = initialValue;

	return curr;
}

int MySemaphoreDestroy(MySemaphore sem)
{
	semaphore *curr = sem;

	if(curr->semaphore_q->size != 0) // This implies there are other threads waiting on the semaphore
	return -1;
	else
	{
		curr->semaphore_q = NULL; // Point it to NULL to deallocate it
		return 0;
    	}
}


void MySemaphoreWait(MySemaphore sem)
{
	//save input into temp semaphore type variable
	semaphore *curr_s = sem;
	MyThreadInfo *curr_t = running_t;
	// decrement the value
	curr_s->final_value--;	
	enqueue(curr_s->semaphore_q, curr_t);
	if(ready_q->size > 0)
	{
		running_t = set_to_next(ready_q);
		ready_q->size--;
		if(running_t == main_t)
		{
			enqueue(ready_q,running_t);
			ready_q->size--;		
			running_t = set_to_next(ready_q);	
		}
	getcontext(&curr_t->context);
	swapcontext(&curr_t->context, &running_t->context);
	}
	else
	{
		running_t = main_t;
		swapcontext(&curr_t->context, &main_t->context);
	}
}


void MySemaphoreSignal(MySemaphore sem)
{
	//save input into temp semaphore type variable
	semaphore *curr_s = sem;
	MyThreadInfo *curr_t = (MyThreadInfo*) malloc(sizeof(MyThreadInfo));
	// increment the value
	if(curr_s->final_value >= curr_s->initial_value) // we cannot increase the value more 
	{
		return;
	}
	curr_s->final_value++;	
	if(curr_s->semaphore_q->size >0)
	{
		curr_t = set_to_next(curr_s->semaphore_q);
		curr_s->semaphore_q->size--;
		enqueue(ready_q, curr_t );
	}
}
