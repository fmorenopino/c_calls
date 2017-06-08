/* Unicast_multicast communication
THIS IS CODE for host2
compile with
	gcc -Wall -o host2 mcast_example_host2.c
execute as
	./host2
	
host1 uses an unicast local address.
host2 receives data in a multicast address. It receives from ANY sender.
(If you are testing with many of your mates, you should CHANGE the multicast address to avoid collisions)

host2 should be started first, then start host1. 
	packet1: host1 (unicast) -> host2(multicast)
	packet2: host2 (unicast) -> host1(unicast)
In all cases, PORT is used both as source and destination (just as stated in RFC 4961)

host1 and host2 can be executed in any host with multicast connectivity between them (preferrably from the same link, 163.117.144/24	
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <strings.h>

#define MAXBUF 256
#define PORT 5000

/* change the following multicast address if you want to avoid collisions. Note that you must change it here AND ALSO in host1 code */
#define GROUP "225.0.1.1"
const char message[16]= "Sent from host2";

int main(void) {
  int s, r; /* s for socket, r for storing results from system calls */
  struct sockaddr_in local, rem; /* to build address/port info for local node and remote node */ 
  struct ip_mreq mreq; /* for multicast configuration */
  char buf[MAXBUF]; /* to receive data from remote node */
  socklen_t  from_len; /* to store the length of the address returned by recvfrom */

  /* preparing bind */
  bzero(&local, sizeof(local));
  local.sin_family = AF_INET;
  local.sin_port = htons(PORT); /* besides filtering, this assures that info is being sent with this PORT as local port */
  
  if (inet_pton(AF_INET, GROUP, &local.sin_addr) < 0) {
    perror("inet_pton");
    return 1;
  }
  
  /* creating socket */
  if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return 1;
  }

  /* binding socket - using mcast local address */
  if (bind(s, (struct sockaddr *)&local, sizeof(local)) < 0) {
    perror("bind");
    return 1;
  }

/* setsockopt configuration for joining to mcast group */
  if (inet_pton(AF_INET, GROUP, &mreq.imr_multiaddr) < 0) {
    perror("inet_pton");
    return 1;
  }
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);

  if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
    perror("setsockopt");
    return 1;
  }


  from_len = sizeof (rem); /* remember always to set the size of the rem variable in from_len */	
  if ((r = recvfrom(s, buf, MAXBUF, 0, (struct sockaddr *) &rem, &from_len)) < 0) {
    perror ("recvfrom");
    } else {
    	    buf[r] = 0; /* convert to 'string' by appending a 0 value (equal to '\0') after the last received character */
    	    printf("Host2: Received message in multicast address. Message is: %s\n", buf); fflush (stdout);
    }
    
 
/* Note that we use the information of the remote node filled in by the 'recvfrom' call (the rem structure), as is, for the sendto call. This means that we are sending to the same node from which we received a packet in the recvfrom call. This is how host2 'learns' the unicast address and port of host1. */  

/* Using sendto to send information. Since I've made a bind to the socket, the local (source) port of the packet is fixed. 
In the rem structure I have the address and port of the remote host, as returned by recvfrom */ 
if ( (r = sendto(s, message, sizeof(message), /* flags */ 0, (struct sockaddr *) &rem, sizeof(rem)))<0) {
	perror ("sendto");
    } else {
      buf[r] = 0;
      printf("Host2: Sent message to UNIcast address\n"); fflush (stdout);
    }
 
 
return 0; /* everything ended fine */  

}   
