/*******************************************************/
/* confArgs.h */
/*******************************************************/

/* Captures arguments for conf application */



/* captures arguments from command line. Initial values of the variables provided are not relevant for its operation */
void  captureArguments (	int argc, char *argv[], 
	int *operation,  /* Returns equested mode of operation, first or second, using the values defined in conf.h: enum operat {FIRST, SECOND}; */
	char *firstIp, /* For 'second' mode only: returns the requested address of node 'first' (for 'first' mode, it returns an empty string; 'first' should no use this value). Note that this function does not check if the string provided by the user has a valid IP address format (xxx.yyy.zzz.kkk) */  
	char *firstMulticastIP, /* For 'first' mode only: returns the requested MULTICAST address of node 'first' if -m option is used. It returns an empty string if -m is not used in 'first' mode, or if the code is started in 'second' mode. */
	int *port, /* Both modes: returns the port requested to be used in the communication */ 
	int *vol, /* Both modes: returns the volume requested (for both playing and recording). Value in range [0..100] */
	int *packetDuration,  /* Both modes: returns the requested duration of the playout of an UDP packet. Measured in ms */
	int *verbose, /* Both modes: returns if the user wants to show detailed traces (value 1) or not (value 0) */
	int *payload, /* For 'second' mode only: returns the requested payload for the communication. This is the payload to include in RTP packets. Values defined in conf.h: enum payload {PCMU=100,  L16_1=11}. Therefore, in command line either number 100 or number 11 are expected. (set to 0 in 'first' mode, must not be used) */
	int *bufferingTime /*  Both modes: returns the buffering time requested before starting playout. Time measured in ms. */
	);

/* prints current values - can be used for debugging */
void  printValues (int operation, const char * firstIp, const char * firstMulticastIP, int port, int vol, int packetDuration, int verbose, int payload, int bufferingTime );



