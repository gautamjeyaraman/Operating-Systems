#include "mythread.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <malloc.h>
#define STACKSIZE 8192

//Constant for tracking threadID's
static int threadID = 0;

// Start:Thread structure with context object, thread's unique ID , type of join applied and Blocked by which thread
struct ThreadInfo{
	ucontext_t* context;
	int threadID;
	struct Queue* childrenThread;
	// 0:Join,1:JoinAll,2:None
	int typeOfJoin;	
	struct ThreadInfo* blockedBy;
};
// End:Thread structure with context object, thread's unique ID , type of join applied and Blocked by which thread

// Start:Semaphore structure with InitialValue , BlockedQueue
struct SemaphoreInfo{
	int initializedValue;	
	struct Queue* blockedQueue;
};
// Start:Semaphore structure with InitialValue , BlockedQueue

//Queue Implementation : Starts

// Declaring Node structure
struct Node{
	struct ThreadInfo* thread;
	struct Node* next;
};

// Declaring Queue structure
typedef struct Queue {
	struct Node *front;
	struct Node *rear;
} Queue;

// Declaring Ready and Blocked Queue
Queue* rQueue = NULL;
Queue* bQueue = NULL;

// Adding new thread to the specified Queue
void enqueue(Queue* q,struct ThreadInfo* newThread)
{
	struct Node* newNode = NULL ;
        newNode = malloc(sizeof (struct Node));
	newNode->thread = newThread;
	
	if (q->front == NULL & q->rear == NULL){
		q->front = newNode;
		q->rear = newNode;		
		newNode->next = NULL;
	}
	else{
		q->rear->next = newNode;	
		q->rear = newNode;			
		newNode->next = NULL;
	}
}


// Deleting thread from front of the specified Queue
struct ThreadInfo* dequeue(Queue* q){
	struct Node* tempFront = NULL;
	struct ThreadInfo* threadDequed = NULL;	
	if(q->front == NULL){	
		return NULL;
	}
	else if(q->front == q->rear){
		tempFront = q->front;
		threadDequed = tempFront->thread;
		q->front = NULL;
		q->rear = NULL;
	}
	else{
		tempFront = q->front;
		threadDequed = tempFront->thread;
		q->front = q->front->next;
	}
	free(tempFront);
	return threadDequed;
}

// Remove a particular thread from specified Queue
struct ThreadInfo* removeThread(Queue* q,struct ThreadInfo* thread){
	struct Node* threadPointer = q->front;
	struct Node* previousThread = q->front;
	while(threadPointer !=NULL){
		if(threadPointer->thread == thread){
			if(threadPointer == q->front && threadPointer == q->rear){
				q->front = NULL;
				q->rear = NULL;
			}
			else{ 
				if(threadPointer == q->front)	
					q->front = threadPointer->next;
				else if(threadPointer == q->rear) {
					q->rear = previousThread;
					previousThread->next = NULL;
				}	
				else{	
					previousThread->next = threadPointer->next;
				}
				free(threadPointer);
			}
		}
		else
		{
			previousThread = threadPointer;
			threadPointer = threadPointer->next;
		}	
	}
}

// Displaying the Queue
void displayQueue(Queue* q){
	struct Node* threadPointer = q->front;
	printf("\n");
	while(threadPointer !=NULL){
		printf("\n%d", threadPointer->thread->threadID);
		threadPointer = threadPointer->next;
	}
	
}


// Check whether thread is present or not in specified Queue --  If present return 1 else 0
int scanQueue(Queue* q,struct ThreadInfo* thread){
	struct Node* threadPointer = q->front;
	while(threadPointer !=NULL){
		if (threadPointer->thread == thread){
			return 1;
		}
		threadPointer = threadPointer->next;
	}
	return 0;
}

//Queue Implementation : Ends


// Global varibles for unix process and current thread
struct ThreadInfo* unixProcessThread = NULL;
struct ThreadInfo* currentThread = NULL;

//Thread functionality Starts:
void MyThreadInit(void(*start_funct)(void *), void *args){
	//Initializing ready and blocked Queue
	rQueue = (Queue *)malloc(sizeof(Queue));
	bQueue = (Queue *)malloc(sizeof(Queue));
	rQueue->front = NULL;
	rQueue->rear = NULL;
	bQueue->front = NULL;
	bQueue->rear = NULL;
	//Getting context of main thread and saving it
	unixProcessThread = malloc (sizeof(struct ThreadInfo));	
	unixProcessThread->context = (ucontext_t *) malloc(sizeof(ucontext_t));	
	//Allocating memory to firstThread and providing attribute values	
	currentThread = malloc (sizeof(struct ThreadInfo));	
	currentThread->context = (ucontext_t *) malloc(sizeof(ucontext_t));
        getcontext(currentThread->context);
	currentThread->context->uc_stack.ss_sp = malloc(STACKSIZE);
 	currentThread->context->uc_stack.ss_size = STACKSIZE;
 	currentThread->context->uc_link= NULL;
	threadID = threadID+1;
	currentThread->threadID = threadID;
	currentThread->childrenThread = (Queue *)malloc(sizeof(Queue));
	currentThread->childrenThread->front = NULL;
	currentThread->childrenThread->rear = NULL;
	currentThread->typeOfJoin = 2;	
	currentThread->blockedBy = NULL;
        //Making firstThread context
	makecontext(currentThread->context, (void *)start_funct,1,args);
        //Swapping mainThread context with firstThread context
	swapcontext(unixProcessThread->context, currentThread->context);	
	return;
}

MyThread MyThreadCreate(void(*start_funct)(void *), void *args){
	//Creating child thread of curently executing thread and adding providing its attributes
	struct ThreadInfo* childThread = NULL;
	childThread = malloc (sizeof(struct ThreadInfo));	
	childThread->context = (ucontext_t *) malloc(sizeof(ucontext_t));
        getcontext(childThread->context);
	childThread->context->uc_stack.ss_sp = malloc(STACKSIZE);
 	childThread->context->uc_stack.ss_size = STACKSIZE;
 	childThread->context->uc_link= currentThread->context;
	threadID = threadID+1;
	childThread->threadID = threadID;
	childThread->childrenThread = (Queue *)malloc(sizeof(Queue));
	childThread->childrenThread->front = NULL;
	childThread->childrenThread->rear = NULL;
	childThread->typeOfJoin = 2;	
	childThread->blockedBy = NULL;
	makecontext(childThread->context, (void *)start_funct,1,args);
	enqueue(currentThread->childrenThread,childThread);
	enqueue(rQueue,childThread);
	return (MyThread)childThread;
}

void MyThreadYield(void){
	struct ThreadInfo* currThread = currentThread;
	//enqueue currentthread to end of queue and get the next thread from ready queue
	enqueue(rQueue, currentThread);
	currentThread = dequeue(rQueue);	
	if(currentThread != NULL){
		swapcontext(currThread->context,currentThread->context);
	}
}

int MyThreadJoin(MyThread thread){
	struct ThreadInfo* blockingThread = (struct ThreadInfo*)thread;
	struct ThreadInfo *curThread = currentThread;
	// Checking whether passed thread is immediate child or not
	int immeChild = scanQueue(currentThread->childrenThread,blockingThread);
	if(immeChild == 1){
		// Checking whether passed thread is in readyQueue and waiting for its turn
		int scanResult = scanQueue(rQueue,blockingThread);
		if(scanResult == 1){
			curThread->blockedBy = blockingThread;				
			curThread->typeOfJoin = 0;
			enqueue(bQueue, curThread);
			currentThread = dequeue(rQueue);	
			if(currentThread != NULL){
				swapcontext(curThread->context,currentThread->context);
			}
		}
	}
	else
	{
		return -1;
	}
	return 0;
}

void MyThreadJoinAll(void){
	if(currentThread->childrenThread->front != NULL && currentThread->childrenThread->rear != NULL) {
		struct ThreadInfo *curThread = currentThread;
		curThread->typeOfJoin = 1;
		enqueue(bQueue, curThread);
		currentThread = dequeue(rQueue);	
		if(currentThread != NULL){
			swapcontext(curThread->context,currentThread->context);
		}
	}
}	

void updateChildrenAndFreeParent(struct ThreadInfo* current){
	struct Node* currThread = bQueue->front;
	while(currThread != NULL){
		removeThread(currThread->thread->childrenThread,current);
		struct Node* childThread = currThread->thread->childrenThread->front;
		if(currThread->thread->typeOfJoin == 1){
			// If no children in thread's queue and type of join is JoinALL
			if(currThread->thread->childrenThread->front == NULL && currThread->thread->childrenThread->rear == NULL) 				{	
				currThread->thread->typeOfJoin = 2;				
				enqueue(rQueue,currThread->thread);				
				removeThread(bQueue,currThread->thread);
				
			}
		}
		else if(currThread->thread->typeOfJoin == 0)
		{			
			// If blocked queue in thread's blocking queue and type of join is Join
			if(currThread->thread->blockedBy == current)
			{	
				currThread->thread->typeOfJoin = 2;				
				enqueue(rQueue,currThread->thread);	
				removeThread(bQueue,currThread->thread);			
			}
		}
		currThread = currThread->next;	
	}
}

void MyThreadExit(void){
	struct ThreadInfo* threadNow = currentThread;
	updateChildrenAndFreeParent(threadNow);
	free(currentThread);
	currentThread = dequeue(rQueue);		
	if(currentThread != NULL){
		setcontext(currentThread->context);
	}
	else{
		setcontext(unixProcessThread->context);
	}	
}

// Create a semaphore by initializing the queue with initial value and empty blocking queue
MySemaphore MySemaphoreInit(int initialValue){
	struct SemaphoreInfo* newSemaphore = NULL;	
	newSemaphore = malloc(sizeof (struct SemaphoreInfo));
	newSemaphore->initializedValue = initialValue;
	newSemaphore->blockedQueue = (Queue *)malloc(sizeof(Queue));
	newSemaphore->blockedQueue->front = NULL;
	newSemaphore->blockedQueue->rear = NULL;
	return (MySemaphore)newSemaphore;
}

// Signal a semaphore : by incrementing the initialized value by 1 and if initialized value is > 0 then removing thread from blocked queue.
void MySemaphoreSignal(MySemaphore sem){
	struct SemaphoreInfo* passedSemaphore = (struct SemaphoreInfo*)sem;
	passedSemaphore->initializedValue = passedSemaphore->initializedValue + 1;
	if(passedSemaphore->initializedValue <= 0) {
		struct ThreadInfo* blockedThread = dequeue(passedSemaphore->blockedQueue);
		enqueue(rQueue, blockedThread);
	}
}

// Wait on a semaphore : by decrementing the initialized value by 1 and if initializedValue < 0 then add the thread to blocking queue
void MySemaphoreWait(MySemaphore sem){
	struct SemaphoreInfo* passedSemaphore = (struct SemaphoreInfo*)sem;		
	passedSemaphore->initializedValue = passedSemaphore->initializedValue - 1;
	if(passedSemaphore->initializedValue < 0)
	{
		enqueue(passedSemaphore->blockedQueue, currentThread);
		struct ThreadInfo* currThread = currentThread;
		currentThread = dequeue(rQueue);	
		if(currentThread != NULL){
			swapcontext(currThread->context, currentThread->context);
		}
	}
}

// Destroy on a semaphore : Freeing the resources
int MySemaphoreDestroy(MySemaphore sem){	
	struct SemaphoreInfo* passedSemaphore = (struct SemaphoreInfo*)sem;
	if(passedSemaphore->blockedQueue->front == NULL && passedSemaphore->blockedQueue->rear == NULL)
	{
		free(sem);
		free(passedSemaphore->blockedQueue);	
		free(passedSemaphore);
		return 0;
	}
	else{
		return -1;	
	}
}
