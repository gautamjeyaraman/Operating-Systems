/******************************************************************************
 *
 *  File Name........: master.c
 *
 *  Description......:
 *  Creates a program that establishes a passive socket.  The socket 
 *  accepts connection from the player.c program.  While the connection is
 *  open, listen will print the characters that come down the socket.
 *
 *  Listen takes a single argument, the port number of the socket.  Choose
 *  a number that isn't assigned.  Invoke the speak.c program with the 
 *  same port number.
 *
 *  Revision History.:
 *
 *  When  			  Who         What
 *  03/29/2016    vin/Nitish     Created
 *
 *****************************************************************************/

/*........................ Include Files ....................................*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

// Structure to store players info on master side
struct players {
  int playerID;   
  int leftPlayerID;
  int leftPlayerPort;
  struct sockaddr_in leftPlayerHost;
  int rightPlayerID;
  int rightPlayerPort;
  struct sockaddr_in rightPlayerHost;
  struct sockaddr_in hostInfo;
  int playerPort;
  int fileDesc;
};
struct players *playersArr;

static struct garamAloo {
	char alooTrace[10001];
	int remainingHops;
}hotAloo;

// Driver function for master
main (int argc, char *argv[])
{
  char buf[512];
  char host[64];
  int s, p, fp, rc, len, port, playersCount, numOfHops, temp, i, portInt, sockfd, leftP, j, msg, hops;
  struct hostent *hp, *ihp;
  struct sockaddr_in sin, incoming, temp_addr, ptopin;
  
  // Error if less number of arguments are passed
  if (argc < 4) {
     fprintf(stderr, "Usage: %s <port-number> <number-of-players> <hops>\n", argv[0]);
     exit(1);
  }
  
  // Reading input from command line
  port = atoi(argv[1]);
  playersCount = atoi(argv[2]);
  numOfHops = atoi(argv[3]);
  playersArr = (struct players*) malloc (sizeof(struct players) * playersCount);
  
  // Error when number of players are less than 2.
  if (playersCount <= 1) {
    fprintf(stderr, "Number of players cannot be less than 2.\n");
    exit(1);
  }

  // Ports below 1024 and above 65535 are reserved.
  if (port < 1024 || port > 65535) {
    fprintf(stderr, "Port numbers below 1024 and above 65535 are reserved.\n");
    exit(1);
  }
        
  // Error when number of hops are less than 0      
  if ( numOfHops < 0 ) 
  {
      fprintf(stderr, "Number of hops cannot be negative.\n");
      exit(1);
  }       

  // Getting host name and populating in hostant structure
  gethostname(host, sizeof host);
  hp = gethostbyname(host);
  if ( hp == NULL ) {
    fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
    exit(1);
  }

  // Printing info of master hostname , number of players and number of hops to be executed.
  printf("Potato Master on %s\n",hp->h_name);
  printf("Players = %d\n",playersCount);
  printf("Hops = %d\n",numOfHops);

  // Socket creation using address family INET and STREAMing sockets (TCP) */
  s = socket(AF_INET, SOCK_STREAM, 0);
  if ( s < 0 ) {
    perror("Error in creating port:");
    exit(s);
  }

  // Set up the address and port for port
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  memcpy(&sin.sin_addr, hp->h_addr_list[0], hp->h_length);

  // Bind socket s to address sin 
  rc = bind(s, (struct sockaddr *)&sin, sizeof(sin));
  if ( rc < 0 ) {
    perror("Error in binding port:");
    exit(rc);
  }

  // Listen through the socket
  rc = listen(s, 5);
  if ( rc < 0 ) {
    perror("Error in listening port:");
    exit(rc);
  }

  // Accepting player connections and populating player structure on master side.
  for (i = 0; i < playersCount; i++)
  {
    len = sizeof(sin);
    p = accept(s, (struct sockaddr *)&incoming, &len);
    if ( p < 0 ) {
      perror("Error in binding port:");
      exit(rc);
    }
    playersArr[i].playerID = i;
    memcpy(&(playersArr[i].hostInfo), &incoming, sizeof(struct sockaddr_in)); 
    ihp = gethostbyaddr((char *)&incoming.sin_addr,sizeof(struct in_addr), AF_INET);
    
    printf("Player %d on %s\n", playersArr[i].playerID, ihp->h_name);
    playersArr[i].fileDesc = p;
    playersArr[i].rightPlayerID = (i+1)%playersCount;
    if(i == 0)
    {
    	playersArr[i].leftPlayerID = playersCount-1;
    }
    else
    {
    	playersArr[i].leftPlayerID = i-1;
    }

  	len = recv(p, &portInt, sizeof(int), 0);
    if ( len < 0 ) {
       perror("Error in receiving data::");
       exit(1);
    }

    playersArr[i].playerPort = portInt;
  }
  

  for(i=0;i<playersCount;i++)
  {
  	playersArr[i].rightPlayerPort = playersArr[playersArr[i].rightPlayerID].playerPort; 	
  	playersArr[i].leftPlayerPort = playersArr[playersArr[i].leftPlayerID].playerPort; 
  	playersArr[i].rightPlayerHost = playersArr[playersArr[i].rightPlayerID].hostInfo; 	
  	playersArr[i].leftPlayerHost = playersArr[playersArr[i].leftPlayerID].hostInfo; 
  } 

  // Sending information to all the players 
  for(i=0;i<playersCount;i++)
  {
  	len = send(playersArr[i].fileDesc,&playersArr[i].playerID, sizeof(int), 0);
  	if (len != sizeof(int) ) {
  	     perror("Error in sending data:");
  	     exit(1);
  	}

    len = send(playersArr[i].fileDesc,&playersArr[i].leftPlayerID, sizeof(int), 0);
  	if (len != sizeof(int) ) {
  	     perror("Error in sending data:");
  	     exit(1);
  	}

    len = send(playersArr[i].fileDesc,&playersArr[i].rightPlayerID, sizeof(int), 0);
  	if (len != sizeof(int) ) {
  	     perror("Error in sending data:");
  	     exit(1);
  	}

    len = send(playersArr[i].fileDesc,&playersArr[i].leftPlayerPort, sizeof(int), 0);
  	if (len != sizeof(int) ) {
  	     perror("Error in sending data:");
  	     exit(1);
  	}

    len = send(playersArr[i].fileDesc,&playersArr[i].rightPlayerPort, sizeof(int), 0);
  	if (len != sizeof(int) ) {
  	     perror("Error in sending data:");
  	     exit(1);
  	}

  	len = send(playersArr[i].fileDesc,&playersArr[i].playerPort, sizeof(int), 0);
  	if (len != sizeof(int) ) {
  	     perror("Error in sending data:");
  	     exit(1);
  	}

  	len = send(playersArr[i].fileDesc,&playersArr[i].hostInfo, sizeof(struct sockaddr_in), 0);
  	if (len != sizeof(struct sockaddr_in) ) {
  	     perror("Error in sending data:");
  	     exit(1);
  	}

  	len = send(playersArr[i].fileDesc,&playersArr[i].leftPlayerHost, sizeof(struct sockaddr_in), 0);
  	if (len != sizeof(struct sockaddr_in) ) {
  	     perror("Error in sending data:");
  	     exit(1);
  	}

  	len = send(playersArr[i].fileDesc,&playersArr[i].rightPlayerHost, sizeof(struct sockaddr_in), 0);
  	if (len != sizeof(struct sockaddr_in) ) {
  	     perror("Error in sending data:");
  	     exit(1);
  	}
    
    len = send(playersArr[i].fileDesc,&numOfHops, sizeof(int), 0);
  	if (len != sizeof(int) ) {
  	     perror("Error in sending data:");
  	     exit(1);
  	}
    
    len = send(playersArr[i].fileDesc,&playersCount, sizeof(int), 0);
  	if (len != sizeof(int) ) {
  	     perror("Error in sending data:");
  	     exit(1);
  	}
  } 
  
  // Signal to start the interconnections for players i.e. left and right connections to a player
  for(i=0;i<playersCount;i++)
  {
    msg = -2;
    len = send(playersArr[i].fileDesc,&msg, sizeof(int), 0);
    if (len != sizeof(int) ) {
         perror("send");
         exit(1);
    } 
    sleep(2);
  }

  // Receive signal that all players are now inter connected.
  for(i=0;i<playersCount;i++)
  {
    len = recv(playersArr[i].fileDesc, &msg, sizeof(int), 0);
    if ( len < 0 ) {
       perror("receive all players:");
       exit(1);
    }
  }
  
  // If number of hops > 0 then make the sockets unblocking
  if(numOfHops>0)
  {
	  sleep(1);
	  // Potato creation with number of hops on first position
	  hotAloo.remainingHops = numOfHops;
	  int status;
	  status = fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
	  if (status == -1) 
	  {
	  	perror("calling fcntl");
	  }
	  // Make all the master - player sockets unblocking on master side
	  for(i=0; i<playersCount;i++)
	  {
		  status = fcntl(playersArr[i].fileDesc, F_SETFL, fcntl(playersArr[i].fileDesc, F_GETFL, 0) | O_NONBLOCK);
		  if (status == -1) 
		  {
		  	perror("calling fcntl");
		  }
	  }
	  // Generate a player randomly from all the players and send to him/her
	  srand(playersCount);
	  int first = rand() % (playersCount -1);
	  printf ("All players present, sending potato to player %d\n", first);
	  len = send (playersArr[first].fileDesc, &hotAloo, sizeof(hotAloo),0);
	  if ( len < 0 ) {
	    perror("send potato");
	    exit(1);
	  }

	  // Receive the potato after successful execution of all the hops
	  while (1)
	  {
		  len =0;
		  for (i=0; i<playersCount;i++)
		  {
		  	  len = recv(playersArr[i].fileDesc, &hotAloo, sizeof(hotAloo),0);
		  
			  if (len>0) 
			  {
			  	// Printing the trace of the potatos and on successful printing Close player connections on master side as well as close master socket
			  	printf("Trace of potato: \n");
			  	for (i=0; i<numOfHops-1;i++)
			  	{
		        	printf("%d,",hotAloo.alooTrace[i]);
		        }
		        printf("%d",hotAloo.alooTrace[numOfHops-1]);
		        printf("\n");
		      	for (i=0; i<playersCount;i++)
		      	{
		   	  	  len = send (playersArr[i].fileDesc, &hotAloo, sizeof(hotAloo),0);
		   	  	  close(playersArr[i].fileDesc);
			  	}
			  	close(s);
			  	exit(0);
			  }
		  }
	  }
  }
  else
  {
  	// Close player connections on master side as well as close master socket
  	for (i=0; i<playersCount;i++)
	{
	  	  close(playersArr[i].fileDesc);
	}
	close(s);
	exit(0);	
  }
}
/*........................ end of master.c ..................................*/
