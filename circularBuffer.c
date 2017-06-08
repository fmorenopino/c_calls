/*******************************************************/
/* circularBuffer.c */
/*******************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include "circularBuffer.h"

#ifndef CALL
#define CALL(v,m) {if ( (v)==-1) {perror (m); printf ("Error number: %d, %s\n", errno, strerror (errno)); exit (1); }};
#endif

/* returns a pointer to the memory used to contain the circular buffer (including some memory space used for control). This function ALLOCATES the memory used by the circular buffer. numberOfZones is the number of blocks for the buffer. blockSize is the size in bytes of the buffer */
/* It is possible to manage many buffers for the same application - provided that this code is not accessed simultaneously*/

void * createCircularBuffer (int numberOfBlocks, int blockSize)
{

void * buffer;
int * pointerToInt;

/* structures to manage the the buffer: 5 integers at the beginning 
- number of blocks of the buffer
- size of each block
- index (0 to [numberOfBlocks - 1]) pointing to the first spare block
- index pointing to the first full block
- number of filled blocks  */
  

   if ( (buffer= malloc (numberOfBlocks * blockSize  + 5 * sizeof (int)) )== NULL) 
      {
         printf ("Error reserving memory in circularBuffer\n");
         exit (1);
      }


  /* initiallizing structure */
  pointerToInt = (int *) buffer;
  *(pointerToInt ) = numberOfBlocks;
  *(pointerToInt + 1) = blockSize;
  *(pointerToInt + 2) = 0; 
  *(pointerToInt + 3) = 0; 
  *(pointerToInt + 4) = 0; 

  return buffer;
}


/* returns a pointer to the first ("empty") available block to write on it, or NULL if there are no blocks (be sure that this case is considered in your code)  */
void * pointerToInsertData (void * buffer)
{
  int * ptrNextFreeBlock, * ptrBlockNumber, * ptrBlockSize; 
  int * ptrFullBlockNmb;
  int * returnPtr;

  ptrBlockNumber = (int *) buffer;
  ptrBlockSize = (int *) (buffer + 1 * sizeof (int));
  ptrNextFreeBlock = (int *) (buffer + 2 * sizeof (int));
  /* ptrNextFullBlock is not used */
  ptrFullBlockNmb = (int *) (buffer + 4 * sizeof (int));


  returnPtr = buffer + 5 * sizeof (int) + (* ptrNextFreeBlock) * (* ptrBlockSize);

  if ( (*ptrFullBlockNmb) == (* ptrBlockNumber) )
     { /* buffer is full*/
	 return (NULL);
     }
  else
     { /* normal condition  */
       (* ptrNextFreeBlock) = ((* ptrNextFreeBlock) + 1) % (* ptrBlockNumber);  /* updates free block state */
       (*ptrFullBlockNmb) = (*ptrFullBlockNmb) + 1;
        return (returnPtr);
     }
};


/* returns a pointer to the first available block to be read, or NULL if there are no blocks (be sure that this case is considered in your code)  */
void * pointerToReadData (void * buffer)
{
  int *ptrNextFullBlock, * ptrBlockNumber, * ptrBlockSize; 
  int *ptrFullBlockNmb;
  int * returnPtr;

  ptrBlockNumber = (int *) buffer;
  ptrBlockSize = (int *) (buffer + 1 * sizeof (int));
/* ptrNextFreeBlock is not used */
  ptrNextFullBlock = (int *) (buffer + 3 * sizeof (int));
  ptrFullBlockNmb = (int *) (buffer + 4 * sizeof (int));

  returnPtr = buffer + 5 * sizeof (int) + (* ptrNextFullBlock) * (* ptrBlockSize);

  if ( (*ptrFullBlockNmb) == 0)
     { /* circular buffer is empty */
	 return (NULL);
     }
  else
     { /* normal condition */
       (* ptrNextFullBlock) = ((*ptrNextFullBlock) + 1) % (* ptrBlockNumber); 
       (*ptrFullBlockNmb) = (*ptrFullBlockNmb) - 1;
        return (returnPtr);
     }
}


/* frees memory of the buffer. Must be executed before exiting from the process */
void destroyBuffer (void *buffer)
{
  free (buffer);
}
