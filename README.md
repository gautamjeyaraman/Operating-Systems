CSC 501 - Operating-Systems Programming Assignments
---------------------------------------------------

Program – 1
-----------

AIM: Implement a non pre-emptive user-level threading library.

DESCRIPTION: User-level threads library mythread.a will have the following routines:

A) Thread Routines:

  1) MyThread MyThreadCreate (void(*start_funct)(void *), void *args) - This routine creates a new MyThread. The parameter start_func is the function in which the new thread starts executing. The parameter args is passed to the start function. This routine does not pre-empt the invoking thread i.e. the parent (invoking) thread will continue to run; the child thread will sit in the ready queue. 
  
  2) void MyThreadYield(void) - Suspends execution of invoking thread and yield to another thread. The invoking thread remains ready to execute—it is not blocked. Thus, if there is no other ready thread, the invoking thread will continue to execute. 
  
  3) int MyThreadJoin(MyThread thread) - Joins the invoking function with the specified child thread. If the child has already terminated, do not block. A child may have terminated without the parent having joined with it. Returns 0 on success (after any necessary blocking). It returns -1 on failure. Failure occurs if specified thread is not an immediate child of invoking thread.
  
  4) void MyThreadJoinAll(void) - Waits until all children have terminated. Returns immediately if there are no active children. 
  
  5) void MyThreadExit(void) - Terminates the invoking thread.

B) Semaphore Routines:

  1) MySemaphore MySemaphoreInit(int initialValue) - Create a semaphore. Set the initial value to initialValue, which must be non-negative. A positive initial value has the same effect as invoking MySemaphoreSignal the same number of times. On error it returns NULL.
  
  2) void MySemaphoreSignal(MySemaphore sem) - Signal semaphore sem. The invoking thread is not pre-empted. 
  
  3) void MySemaphoreWait(MySemaphore sem) - Wait on semaphore sem. 
  
  4) int MySemaphoreDestroy(MySemaphore sem) - Destroy semaphore sem. Do not destroy semaphore if any threads are blocked on the queue. Return 0 on success, -1 on failure. 

C) Unix Process Routines:

  1) void MyThreadInit (void(*start_funct)(void *), void *args) - This routine is called before any other MyThread call. It is invoked only by the Unix process. It is similar to invoking MyThreadCreate immediately followed by MyThreadJoinAll. The MyThread created is the oldest ancestor of all MyThreads—it is the “main” MyThread. This routine can only be invoked once. It returns when there are no threads available to run (i.e., the thread ready queue is empty. 

Special Notes:
  
  1) FIFO (first-in, first-out) scheduling policy for threads on the ready queue.
  
  2) All routines are available via a library, created by using the ar command.
  
  3) Allocate at least 8KB for each thread stack.
  
  4) Usage: ar rcs mythread.a file1.o file2.o file3.o 
  
  5) The Unix process in which the user-level threads run is not a MyThread. Therefore, it will not be placed on the queue of MyThreads. Instead it will create the first MyThread and relinquish the processor to the MyThread engine.

Program – 2
-----------

AIM: Implement the micro-shell, ush (ie, μ-shell).

DESCRIPTION: 

  1) Summary of details enumerated in the manual page: 
  	rc file (~/.ushrc) 
  	hostname% 
  	special chars:	& | ; < > >> |& >& >>& 
  	backslash 
  	strings 
  	commands: cd echo logout nice pwd setenv unsetenv where 

  2) Major tasks :
	  Command line parsing 
	  ush.tar.gz 
	  Command execution 
	  fork(2) 
	  exec family: execvp(2) might be the best 
	  I/O redirection 
		  open(2), close(2) (not fopen, fclose) 
		  dup(2), dup2(3c) 
		  stdin, stdout, stderr 
	  Environment variable 
		  putenv(3c), getenv(3c) 
	  Signal handling 
		  signal(5) 
		  signal(3c) 

  3) Usage :	 ./ush and then commands on the shell or as an example shown below:
	             ./ush < test.1 > test.1.out 2>&1

Program – 3
----------------

AIM: Use sockets and TCP/IP communication to play a distributed game of "hot potato." 

DESCRIPTION: Hot Potato is a children's game in which a "potato" is rapidly tossed around until, at some arbitrary point, the game ends. The child holding the potato is "it.". We will create a ring of "player" processes that will pass the potato around; therefore, each player will have a left and a right neighbor. Furthermore, we will create a "ringmaster" process that will start each game, report the results, and shut down the network. To begin, the ringmaster creates a potato with some number of hops and sends it to a randomly selected player. Each time a player receives the potato, it will decrement the number of hops and append its identity on the potato. If the remaining number of hops is greater than zero, it will randomly select a neighbor and send the potato to that neighbor. The game ends when the hops counter reaches zero. The player holding the potato sends it to the ringmaster indicating the end of this game. The ringmaster prints a trace of the game (from the identities that are appended to the potato) to the screen, and shuts down the network. The trace consists of the path the potato took (that is, the players in the order they held the potato). Each player process will create three connections: left, right, and ringmaster. The potato can arrive on any of the three connections. Commands and important information may also be received from the ringmaster. The ringmaster will create n connections, one for each player. At the end of a game the ringmaster will receive the potato from the player who is "it.". The programs will use Unix sockets for communication. 

NOTES:

  1) Master Usage:    master <port-number> <number-of-players> <hops>
    	where number-of-players is greater than one and hops is non-negative.

  2) Player Usage:    player <master-machine-name> <port-number>
	    where master-machine-name is the name of the machine on which master was invoked and port-number is the same as was passed to master. 

  3) Players are numbered beginning with 0. The players are connected in the ring such that the left neighbor of player n is player n-1 and the right neighbor is player n+1. (Player 0 is the right neighbor of last player (number_of_players-1). 

  4) Zero (0) is a valid number of hops.

Program – 4
----------------

AIM: Implement an in-memory filesystem (ie, RAMDISK) using FUSE. 

DESCRIPTION: 

  1) FUSE : (Filesystem in Userspace) is an interface that exports filesystem operations to user-space. Thus filesystem routines are executed in 		user-space. Continuing the example, if the file is a FUSE file then operating system upcalls into user space in order to invoke the 		code associated with read.
  
  2) RAMDISK :
  	1) Instead of reading and writing disk blocks, the RAMDISK filesystem will use main memory for storage.
  	2) It is hierarchtical (has directories) and must support standard accesses, such as read, write, and append. However, the filesystem 		is not persistent. 
  
  3) System calls which RAMDISK supports: 
  	open, close 
  	read, write 
  	creat , mkdir 
  	unlink, rmdir 
  	opendir, readdir 
  
  4) Usage : ramdisk <mount_point> <size> 
