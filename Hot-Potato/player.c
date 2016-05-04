/******************************************************************************
 *
 *  File Name........: player.c
 *
 *  Description......:
 *  Create a process that talks to the master.c program.  After 
 *  connecting, each line of input is sent to listen.
 *
 *  This program takes two arguments.  The first is the host name on which
 *  the masyer process is running. (Note: master must be started first.)
 *  The second is the port number on which listen is accepting connections.
 *
 *  Revision History.:
 *
 *  When        	Who         		What
 *  03/29/2016    vin /Nitish        Created
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

#define LEN 64

// Prepareing players structure to store all its information
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
  int leftFileDesc;
  int rightFileDesc;
  int masterPort;
  int noOfHops;
  int playersCount;
}playersArr;

static struct garamAloo {
	char alooTrace[10001];
	int remainingHops;
}hotAloo;

main (int argc, char *argv[])
{
  int s, newSock, rc, len, port, portStart, id, leftSock, rightSock, sockfd, msg, hops, len1, len2, len3;
  char host[LEN], str[LEN];
  struct hostent *hp,*phost,*hostInfor;
  struct sockaddr_in sin,pin,lsin,incoming,ptopin;
  char *connectionString;
  
  // Error if less number of arguments
  if ( argc != 3 ) {
    fprintf(stderr, "Usage: %s <host-name> <port-number>\n", argv[0]);
    exit(1);
  }

  // Get players hostname and populating hostant structure.
  hp = gethostbyname(argv[1]); 
  if ( hp == NULL ) {
    fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
    exit(1);
  }
  
  port = atoi(argv[2]);
  playersArr.masterPort = port;

  // Socket creation using address family INET and STREAMing sockets (TCP) 
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    perror("Error in creation of socket:");
    exit(s);
  }
  
  // Set up the address and port for the socket
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  memcpy(&sin.sin_addr, hp->h_addr_list[0], hp->h_length);

  // Connect to socket at above addr and port 
  rc = connect(s, (struct sockaddr *)&sin, sizeof(sin));
  if ( rc < 0 ) {
    perror("Error in connecting port:");
    exit(rc);
  }
  
  playersArr.fileDesc = s;

  // Generation of socket using bind error if port are not free to bind , if no error then send that 
  // port to master as well as store in player structure as player's port
  newSock = socket(AF_INET, SOCK_STREAM, 0);
  if (newSock < 0) {
    perror("Error in socket creation:");
    exit(newSock);
  } 
  gethostname(host, sizeof host);
  phost = gethostbyname(host);
  if ( phost == NULL ) {
    fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
    exit(1);
  }
  portStart = 1025;
  rc = -1;
  while ((rc < 0) && (portStart < 65535))
  {
	  pin.sin_family = AF_INET;
	  pin.sin_port = portStart;
	  memcpy(&pin.sin_addr, phost->h_addr_list[0], phost->h_length);
	  rc = bind(newSock, (struct sockaddr *)&pin, sizeof(pin));
	  if ( rc < 0 ) 
	  {
			portStart++;
	  }
  }
  if (portStart >= 65535)
  {
	perror("No free ports:");
    exit(rc);
  }
  
  len = send(s, &portStart, sizeof(int), 0);
  if (len < 0) 
  {
    perror("Issue in sending to master:");
    exit(1);
  }
  
  // Receiving information from master to store in player's structure
  len = recv(s, &id, sizeof(int), 0);
  if ( len < 0 ) {
    perror("Issue in receive from master:");
    exit(1);
  }
  playersArr.playerID = id;
  
  len = recv(s, &id, sizeof(int), 0);
  if ( len < 0 ) {
    perror("Issue in receive from master:");
    exit(1);
  }
  playersArr.leftPlayerID = id;
  
  len = recv(s, &id, sizeof(int), 0);
  if ( len < 0 ) {
    perror("Issue in receive from master:");
    exit(1);
  }
  playersArr.rightPlayerID = id;

  len = recv(s, &id, sizeof(int), 0);
  if ( len < 0 ) {
    perror("Issue in receive from master:");
    exit(1);
  }
  playersArr.leftPlayerPort = id;
  
  len = recv(s, &id, sizeof(int), 0);
  if ( len < 0 ) {
    perror("Issue in receive from master:");
    exit(1);
  }
  playersArr.rightPlayerPort = id;

  len = recv(s, &id, sizeof(int), 0);
  if ( len < 0 ) {
    perror("Issue in receive from master:");
    exit(1);
  }
  playersArr.playerPort = id;

  len = recv(s, &lsin, sizeof(struct sockaddr_in), 0);		
  if ( len < 0 ) 
  {
    perror("Issue in receive from master:");
    exit(1);
  }
  memcpy(&(playersArr.hostInfo), &lsin, sizeof(struct sockaddr_in));
  
  len = recv(s, &lsin, sizeof(struct sockaddr_in), 0);	
  if ( len < 0 ) 
  {
    perror("Issue in receive from master:");
    exit(1);
  }
  memcpy(&(playersArr.leftPlayerHost), &lsin, sizeof(struct sockaddr_in));

  len = recv(s, &lsin, sizeof(struct sockaddr_in), 0);		
  if ( len < 0 ) 
  {
    perror("Issue in receive from master:");
    exit(1);
  }
  memcpy(&(playersArr.rightPlayerHost), &lsin, sizeof(struct sockaddr_in));

  len = recv(s, &id, sizeof(int), 0);
  if ( len < 0 ) {
    perror("Issue in receive from master:");
    exit(1);
  }
  playersArr.noOfHops = id;

  len = recv(s, &id, sizeof(int), 0);
  if ( len < 0 ) {
    perror("Issue in receive from master:");
    exit(1);
  }
  playersArr.playersCount= id;

  // Potato creation and initialization
  //hotAloo.remainingHops = playersArr.noOfHops;

  // Receive msg from master to start inter connection for players
  len = recv(s, &msg, sizeof(int), 0);
  if ( len < sizeof(int)) 
  {
    perror("Issue in receive from master:");
    exit(1);
  }

  // Interconnection for player's is created using forking the parent process
  // Child will accept connect request of parent from already bind port
  if(msg == -2)
  {
  	rightSock = socket(AF_INET, SOCK_STREAM, 0);
  	if (rightSock < 0) {
	    exit(rightSock);
	  }
	  sin.sin_family = AF_INET;
	  sin.sin_port = playersArr.rightPlayerPort;
	  memcpy(&sin.sin_addr, &playersArr.rightPlayerHost.sin_addr, sizeof(playersArr.rightPlayerHost.sin_addr));
  	int pid = fork();
  	
  	if(pid < 0)
  	{
  		perror("Error in forking:");
    	exit(1);
  	}
  	else if(pid == 0)
  	{
    	while(1)
  		{
  		  rc = connect(rightSock,(struct sockaddr *)&sin, sizeof(sin));
  	  	  if ( rc < 0 ) {
    		  	continue;
  		    }
  		    else
  		    {
  		    	break;
  		    }
  		}
	  	exit(0);
  	}
  	else
  	{
		rc = listen(newSock, 5);
		if ( rc < 0 ) {
		   perror("Error in listening data:");
		   exit(rc);
		}
     	len = sizeof(pin);
  		leftSock = accept(newSock, (struct sockaddr *)&pin, &len);
  	  	if ( leftSock < 0 ) {
  	  	  perror("Error in accepting data:");
  	      exit(rc);
  	  	}
	    playersArr.leftFileDesc = leftSock;
    }
    playersArr.rightFileDesc = rightSock;
  }

  sleep(1);
  printf("Connected as player %d.\n",playersArr.playerID);

  msg = -1;
  len = send(s, &msg, sizeof(int), 0);  
  if (len < 0) 
  {
    perror("Sending data issue:");
    exit(1);
  }

  // If number of hops are more then do the following:
  // 1. Make all the sockets unblocking
  // 2. Listen from all the sides : master , left side and right side 
  // 3. On receiving potato check whether hops are > 0 or not.
  // 4. Descrease the hops and if hops > -1 then randomly choose left or right side to send the potato.
  // 5. Else send the data to master after printing I'm it. 
  if(playersArr.noOfHops > 0)
  {  
	  	sleep(1);

		int info;
		info = fcntl(playersArr.fileDesc, F_SETFL, fcntl(playersArr.fileDesc, F_GETFL, 0) | O_NONBLOCK);
		if (info < 0) 
		{
			perror("Erorr in fcntl ececution:");
		}
		info = fcntl(playersArr.leftFileDesc, F_SETFL, fcntl(playersArr.leftFileDesc, F_GETFL, 0) | O_NONBLOCK);
		if (info < 0)
		{
			perror("Erorr in fcntl ececution:");	
		} 
		info = fcntl(playersArr.rightFileDesc, F_SETFL, fcntl(playersArr.rightFileDesc, F_GETFL, 0) | O_NONBLOCK);
		if (info < 0) 
		{
			perror("Erorr in fcntl ececution:");
		}

		while(1)
		{
		    len1 =0;
		    len1 = recv(playersArr.fileDesc, &hotAloo, sizeof (hotAloo),0);
		    len2 = 0;
		    len2 = recv(playersArr.leftFileDesc, &hotAloo, sizeof (hotAloo),0);
		    len3 = 0;
		    len3 = recv(playersArr.rightFileDesc, &hotAloo, sizeof (hotAloo),0);

		    if(len1>0 || len2>0 || len3>0)
		    {
		    	char c = playersArr.playerID;
		 		hotAloo.alooTrace[playersArr.noOfHops - hotAloo.remainingHops] = c;
				hotAloo.remainingHops  = hotAloo.remainingHops  - 1;
				
				if (hotAloo.remainingHops < 0) 
		        {
		        	break;
		        }
		      	if (hotAloo.remainingHops == 0)
		      	{
					hotAloo.remainingHops = -1;
					printf("I'm it\n");
					len = send (playersArr.fileDesc, &hotAloo, sizeof (hotAloo),0);
					if ( len < 0 ) 
					{
						perror("Error in sending to master:");
					}
		      	}
		      	else 
		      	{
				    if (rand() % 2 !=0)
				    { 
					   if (playersArr.playerID == playersArr.playersCount-1)
						{
							 printf ("Sending potato to 0\n");
						}      
						else
						{
							printf ("Sending potato to %d\n", (playersArr.rightPlayerID));	
						} 

						len = send (playersArr.rightFileDesc, &hotAloo, sizeof (hotAloo),0);
						if ( len < 0 ) 
						{
							perror("Error in sending potato to right:");
						}

						if(inet_ntoa(playersArr.hostInfo.sin_addr) != inet_ntoa(playersArr.rightPlayerHost.sin_addr))
						{
							sleep(1);
						}
						
				    } 
				    else
				    { 
						if (playersArr.playerID == 0) 
						{
							printf ("Sending potato to %d\n", (playersArr.playersCount-1));
						}
						else 
						{
							printf ("Sending potato to %d\n", (playersArr.leftPlayerID));
						}
						len = send (playersArr.leftFileDesc, &hotAloo, sizeof (hotAloo),0);
						if ( len < 0 )
						{ 
					 		perror("Error in sending potato to left:");
				      	}

						if(inet_ntoa(playersArr.hostInfo.sin_addr) != inet_ntoa(playersArr.leftPlayerHost.sin_addr))
						{
							sleep(1);
						}
					
				    }

		      	}
		    }
		}
  }
  close(sockfd);
  close(rightSock);
  close(leftSock);
  close(newSock);
}
/*........................ end of player.c ...................................*/
