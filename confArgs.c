/*******************************************************/
/* confArgs.c */
/*******************************************************/

/* Captures arguments for 'conf' application */
/* Al alternative to this code is to base it on the Linux standard function 'getopt_long' */

#include "conf.h"
#include "confArgs.h"
#include <stdio.h>

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h> 


void captureFirst (int argc, char *argv[], char * firstMulticastIP, int * port, int *vol, int * verbose, int * bufferingTime);
void captureSecond (int argc, char *argv[], char * firstIP, int * port, int *vol, int * packetDuration, int *payload, int * verbose, int * bufferingTime);


/*=====================================================================*/














void  printValues (int operation, const char * firstIP, const char * firstMulticastIP, int port, int vol,  int packetDuration, int verbose, int payload, int bufferingTime)
{
	if (operation == FIRST) { 
		printf ("FIRST mode.\n");
		if (firstMulticastIP[0] == '\0') {
  	  	  printf ("Unicast IP address requested for first.\n");
  	  	}
  	  	else printf ("Multicast IP address requested for first: %s\n", firstMulticastIP);
  	  	
  	  	printf ("Packet duration %d ms.\n", packetDuration);
  	}		
  	else if (operation == SECOND) {
  		printf ("SECOND mode");
  		if (payload == PCMU)
  		{
  			printf (" with 8 bits, single channel, mu law.\n");
  		}
  		else if (payload == L16_1)
  		{
  			printf (" with PCM 16 bits, singleChannel.\n");
  		}
  		printf ("Address of first node: %s.\n", firstIP);
    
  	}


  printf ("Port used %d.\n", port);
  printf ("Volume %d.\n", vol);
  printf ("Requested buffering time: %d ms.\n", bufferingTime);

  if (verbose)
    {
      printf ("Verbose mode ON.\n");
    }
  else printf ("Verbose mode OFF.\n");

    
};



/*=====================================================================*/
void printHelp (void)
{
  printf ("\nconf first [-pLOCAL_RTP_PORT] [-c] [-vVOL] [-mMULTICAST_ADDR] [-kACCUMULATED_TIME]"
"\nconf second addressOfFirst [-pLOCAL_RTP_PORT] [-c] [-vVOL] [-lPACKET_DURATION] [-kACCUMULATED_TIME] [-yPAYLOAD]\n\n(NOTE: firstName can be either a name or an IP address\n\n");
}





/* Insert default values */

/*=====================================================================*/
void  defaultValues (int *operation, char * firstIP, char * firstMulticastIP, int *port, int *vol, int *packetDuration, int *verbose, int *payload, int * bufferingTime)
{
  (*operation) = FIRST; /* not very useful, but removes gcc warning about unused operation variable */
	
  firstMulticastIP[0] = '\0';
  firstIP[0] = '\0';
  *port = 5004;

  (*vol) = 90;

  (*packetDuration) = 20; /* 20 ms */

  (* payload) = PCMU;
  (* verbose) = 0; 
  (* bufferingTime) = 100; /* 100 ms */
};



/*=====================================================================*/
void  captureArguments (int argc, char *argv[], int *operation, char * firstIP, char * firstMulticastIP, int *port, int *vol, int *packetDuration, int *verbose, int *payload, int *bufferingTime)
{

  if (argc==1)
    { 
    printf ("I need to know if mode is 'first' or 'second'\n\n");	    
      printHelp ();
      exit (1); /* error */
    }


  defaultValues (operation, firstIP, firstMulticastIP, port, vol, packetDuration, verbose, payload, bufferingTime);


  /* the first argument is always the role played by the host */
  if (strcmp ("first", argv[1]) == 0)
    {
      (*operation) = FIRST;
      (*payload)=0; /* set to 0 in 'first' */
      
      if (argc > 2) /* need to obtain more arguments than './conf first' */
	{
	  captureFirst (argc-1, argv, firstMulticastIP, port, vol, verbose, bufferingTime);
	}
    }
  else if (strcmp ("second", argv[1]) == 0)
    {
      (*operation) = SECOND;
      captureSecond (argc-1, argv, firstIP, port, vol, packetDuration, payload, verbose, bufferingTime);
    }

  else 
    {
      printf ("Operation could not be identified: expecting 'first' or 'second'.\n\n");
      printHelp();
      exit (1); /* error */
    }
}



/*=====================================================================*/
void captureFirst (int argc, char *argv[], char * firstMulticastIP, int * port, int *vol, int * verbose, int * bufferingTime)
{


  int index, mostSignificantAddressComponent;
  char character;

  for ( index=2; argc>1; ++index, --argc)
    {
      if ( *argv[index] == '-')
	{ 
	  character= *(++argv[index]);

	    switch (character)
	      { 
	      case 'p': /* RTP PORT for FIRST*/ 
		if ( sscanf (++argv[index],"%d", port) != 1)
		  { 
		    printf ("\n-p must be followed by a number\n");
		    exit (1); /* error */
		  }
 
	
		if (  !  (( (*port) >= 1024) && ( (*port) <= 65535)  ))
		  {	    
		    printf ("\nPort number (-p) is out of the requested range, [1024..65535]\n");
		    exit (1); /* error */
		  }
		break;


	      case 'c': /* VERBOSE  */
		(*verbose) = 1;
		break;


	      
	    case 'v': /* VOLume */ 
		if ( sscanf (++argv[index],"%d", vol) != 1)
		  { 
		    printf ("\n-v must be followed by a number\n");
		    exit (1); /* error */
		  }
 
	
		if (  ! (( (*vol) >= 0) && ( (*vol) <= 100) ))
		  {	    
		    printf ("\nVolume (-v) is out of the requested range, [0..100]\n");
		    exit (1); /* error */
		  }
		break;


	      case 'm': /* address of the second - can be mcast */
		if ( sscanf (++argv[index],"%s", firstMulticastIP) != 1)
		  {
		    printf ("\nSomething should follow -m\n");
		    exit (1); /* error */
		  }
				
		/* checks that this address is multicast. Multicast addresses are in the range 224.0.0.0 through 239.255.255.255 */  
		if ( sscanf ( firstMulticastIP, "%d.", &mostSignificantAddressComponent) != 1)
		  { 
			printf ("\nThe argument following -m does not seem to be an internet address\n");
			exit (1); /* error */
		  }
		if ( (mostSignificantAddressComponent < 224) | (mostSignificantAddressComponent > 239) )
		  {
		    printf ("\nThe argument following -m is out of the multicast address range\n");
		    exit (1); /* error */
		  }	
		break;


	      case 'k': /* Time accumulated in buffers */
		if ( sscanf (++argv[index],"%d", bufferingTime) != 1)
		  { 
		    printf ("\n-k must be followed by a number\n");
		    exit (1); /* error */
		  }
		
		if (  ! ( ((*bufferingTime) >= 0)  ))
		  {	    
		    printf ("\nThe buffering time (-k) must be equal or greater than 0\n");
		    exit (1); /* error */
		  }
		break;


	      default:
	      	printf ("\nI do not understand -%c\n", character);      
		printHelp ();
		exit (1); /* error */

	      }
					
	    }
	 else 
	    { /* argument not starting with '-', reject */
	    	    printf ("Every argument of second must start with '-' \n\n");
	    	  printHelp ();
	    	  exit (1); /* error */
	    }
	
	    	    
	
     
    }
  

};



/*=====================================================================*/
void captureSecond (int argc, char *argv[], char * firstAddress, int * port, int *vol, int * packetDuration, int *payload, int * verbose, int * bufferingTime)
{

  int  maxNameNumber = 1;
  
  int index;
  int nameNumber = 0;
  char character;


  for ( index=2; argc>1; ++index, --argc)
    {
 
     if ( *argv[index] == '-')
	{
	  character= *(++argv[index]);
	  switch (character)
	    { 
	    case 'p': /* PORT */ 
	      if ( sscanf (++argv[index],"%d", port) != 1)
		{ 
		  printf ("\n-p must be followed by a number\n");
		  exit (1); /* error */
		}
	      
	      
	      if (  ! (( (*port) >= 1024) && ( (*port) <= 65535 )))
		{	    
		  printf ("\nPort number is out of the requested range, [1024..65535]\n");
		  exit (1); /* error */
		}
	      break;

	    case 'c': /* VERBOSE  */
	      (*verbose) = 1;
	      break;
	      
	      
	    case 'v': /* VOLume */ 
		if ( sscanf (++argv[index],"%d", vol) != 1)
		  { 
		    printf ("\n-v must be followed by a number\n");
		    exit (1); /* error */
		  }
 
	
		if (  ! (( (*vol) >= 0) && ( (*vol) <= 100) ))
		  {	    
		    printf ("\nVolume (-v) is out of the requested range, [0..100]\n");
		    exit (1); /* error */
		  }
		break;

	      
	    case 'l': /* Packet duration */
	      if ( sscanf (++argv[index],"%d", packetDuration) != 1)
		{ 
		  printf ("\n-l must be followed by a number\n");
		  exit (1); /* error */
		}
	      
	      if (  ! ( ((*packetDuration) >= 0)  ))
		{	    
		  printf ("\nPacket duration (-l) must be greater than 0\n");
		  exit (1); /* error */
		}
	      break;
	      
	      
	     case 'k': /* Accumulated time in buffers */
		if ( sscanf (++argv[index],"%d", bufferingTime) != 1)
		  { 
		    printf ("\n-k must be followed by a number\n");
		    exit (1); /* error */
		  }
		
		if (  ! ( ((*bufferingTime) >= 0)  ))
		  {	    
		    printf ("\nThe buffering time (-k) must be equal or greater than 0\n");
		    exit (1); /* error */
		  }
		break;


          case 'y': /* Initial PAYLOAD */
	      if ( sscanf (++argv[index],"%d", payload) != 1)
		{ 
		  printf ("\n-y must be followed by a number\n");
		  exit (1); /* error */
		}
	      
	      if (  ! ( ((*payload) == PCMU) || ( (*payload) == L16_1)  ))
		{	    
		  printf ("\nUnrecognized payload number. Must be either 11 or 100.\n");
		  exit (1); /* error */
		}
	      break;
	      

	      
	      
	    default:
	      printf ("\nI do not understand -%c\n", character);      
	      printHelp ();
	      exit (1); /* error */
	      
	      
	      
	    }
	}
   

        else /* THERE IS A NAME */
        {
          strcpy (firstAddress, argv[index]);
       
	  nameNumber += 1;
	  
	  if  (nameNumber > maxNameNumber)
	    {
	      printf ("I expected a single non '-' argument, the IP address, and found more\n\n");
	      printHelp ();
	      exit (1); /* error */
	    }
	


        }                  
    }
  
  if (nameNumber != 1)
    {
      printf ("Could not obtain an IP address\n\n");
      printHelp();
      exit (1); /* error */
    }
  
};



	
