/*******************************************************/
/* conf.h */
/*******************************************************/


#ifndef SIMPLECONF_H
#define SIMPLECONF_H

#include <sys/time.h>
#include <unistd.h>

/* -------------------------------------------------------------------------------- */
/* DEFINITIONS */
enum operat {FIRST, SECOND};
enum payload {PCMU=100,  L16_1=11};

#define MaxNameLength 100

#ifndef CALL
#define CALL(v,m) {if ( (v)==-1) {perror (m); printf ("Error number: %d, %s\n", errno, strerror (errno)); exit (1); }};
#endif

/* to be used by first before knowing the length of the audio data */
#define MaxLongBuffer 65536

/* read 'rtp.h', which defines structures that complement the following one */ 


#endif /* SIMPLE_H */



