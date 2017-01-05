// Information for threads created. Is return type of MyThreadCreate


typedef struct MyThreadInfo MyThreadInfo;

typedef struct q_element
{
    MyThreadInfo *thread; // data in current queue_element
    struct q_element *next; // pointer to next queue element
}q_element;


typedef struct MyThreadInfo
{
	struct MyThreadInfo *parent;
	ucontext_t context;
	int joined;	
	int childrunning;
	int childwaiting;
	struct queue *child_q;
	int exited;
}MyThreadInfo;


//using a linked list to make a queue of q_element types
typedef struct queue
{
	int size;
	q_element *head;
	q_element *tail;
}queue;


typedef struct semaphore
{
    int initial_value;
    int final_value;
    struct queue *semaphore_q;
}semaphore;
