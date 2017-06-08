/*
audioSimple record|play [-b(8|16)] [stereo] [-vVOLUME] [-dDURATION] [-fFREQUENCY] fileName

Examples on how the program can be started:
./audioSimple record -b16 audioFile
./audioSimple record audioFile
./audioSimple play -b8 stereo -v90 -f8000 audioFile

To compile, execute
	make
from the command line (provided that Makefile and all the .c and .h files required are in the same directory)

Operations:
- store: reads from sound card and writes to a file
- play: the opposite to 'store'

-b 8 or 16 bits per sample
VOL volume [0..100]
DUR duration in seconds; 0 or unspecified means that Ctrol-C is the only way to stop
FRE sampling frequency in Hz

default values:  8 bits, vol 30, dur 0, sampling frequency 8000, mono

*/


/* -------------------------------------------------------------------------------- */
/* INCLUDES */

#include "audioSimpleArgs.h"
#include "configureSndcard.h"
#include <stdbool.h>



/* -------------------------------------------------------------------------------- */
/* CONSTANTS */

const int BUFFER_SIZE = 4096; /* unit of data to work with: fragment to be used in the soundcard and data to read/write on each request to the soundcard */
const int MaxLongSize = 100; /* max filename size */


void record (int descSnd, int numBytesToRead, const char *fileName, int fragmentSize);
void play (int descSnd, int numBytesToWrite, const char *fileName, int fragmentSize);

/* Duplex is not used, it is just provided as an example */
void duplex (int descSnd, int numBytesToProcess, const char *fileName, int fragmentSize);
 
char *buf;
bool allocatedBuf = false;

/*=====================================================================*/
/* activated by Ctrol-C */
void signalHandler (int sigNum __attribute__ ((unused)))  /* __attribute__ ((unused))   -> this indicates gcc not to show an 'unused parameter' warning about sigNum: is not used, but the function must be declared with this parameter */
{
  printf ("audioSimple was requested to finish\n");
  if (allocatedBuf == true) free (buf);
  exit (-1);
}



/*=====================================================================*/
int main(int argc, char *argv[])
{
  struct sigaction sigInfo; /* signal conf */

  struct structSndQuality datQualitySnd;
  int vol;
  int duration, numBytes;
  char nombFich[MaxLongSize];
  int operation; /* record, play */
  int descriptorSnd;
  int requestedFragmentSize;

  /* we configure the signal */
  sigInfo.sa_handler = signalHandler;
  sigInfo.sa_flags = 0;  
  CALL (sigaction (SIGINT, &sigInfo, NULL), "Error installing signal"); 

  /* obtain values from the command line - or default values otherwise */
  captureArguments (argc, argv, &operation, &datQualitySnd, &vol, &duration, nombFich);


  /* soundcard configuration */
  descriptorSnd = 0; /* this is required to request configSnd to open the device for the first time */

  /* create snd descritor and configure soundcard to given format, frequency, number of channels. We also request setting the fragment to BUFFER_SIZE size - when BUFFER_SIZE is set to a power-of-two value, the configured fragment size - returned in 'requestedFragmentSize' should be equal to BUFFER_SIZE */
  requestedFragmentSize = BUFFER_SIZE;
  configSndcard (&descriptorSnd, &datQualitySnd, &requestedFragmentSize); 
  vol = configVol (datQualitySnd.channels, descriptorSnd, vol);
   

  /* obtained values -may differ slightly - eg. frequency - from requested values */
  printValues (operation, datQualitySnd, vol, duration, nombFich);
  printFragmentSize (descriptorSnd);
  printf ("Duration of each packet exchanged with the soundcard :%f\n", (float) requestedFragmentSize / (float) ((datQualitySnd.channels) * (datQualitySnd.format /8) * (datQualitySnd.freq)));


  numBytes = duration * (datQualitySnd.channels) * (datQualitySnd.format /8) * (datQualitySnd.freq); /* note that this is the actual frequency provided by the device */

  /*En la ecuación de arriba: duration sería nuestro packet duration (-l), una vez calculado el valor de
  numBytes, éso es lo que debemos pasarle a fragmentsize)
  */

  if (operation == RECORD)
    record (descriptorSnd, numBytes, nombFich, requestedFragmentSize); /* this function - and the following fucntions - are coded in configureSndcard */
  else if (operation == PLAY)
    play (descriptorSnd, numBytes, nombFich, requestedFragmentSize);
  /* else if (operation == DUPLEX)
    duplex (descriptorSnd, numBytes, nombFich, requestedFragmentSize); */

  close (descriptorSnd);

  return 0;
};
 

 
/*=====================================================================*/
/* this funcion creates a new file fileName. It reads numBytesToRead bytes from descSnd and stores it in the file opened. The read operation is performed in 'fragmentSize' chunks. If numBytesToRead is 0, it reads forever (in this case the application calling should handle the SIGINT signal to orderly stop the operation)
If an error is found in the configuration of the soundcard, the process is stopped and an error message reported. */  
void record (int descSnd, int numBytesToRead, const char * fileName, int fragmentSize)
{
  int file;
  int cycles = 0;
  int maxCycles = numBytesToRead / fragmentSize; /* we round down the number of bytes to record to fit with the requested fragmentSize */
  int status;
  
  /* Creates buffer to store the audio data */

  buf = malloc (fragmentSize); 
  if (buf == NULL) { printf("Could not reserve memory for audio data.\n"); exit (-1); /* very unusual case */ }
  allocatedBuf = true;
  
  /* opens file for writing */
  CALL (file = open  (fileName, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU), "Error creating file for writing");


  /* If it would be needed to store inside the file some data to represent the audio format, this should be the place */
  
  printf("Recording in blocks of %d bytes... :\n", fragmentSize);
  
  while ( (numBytesToRead == 0) || (cycles < maxCycles)) 
    { /* until duration, or Ctrl-C */
      /* reading blocks until there are enough data */
      status = read (descSnd, buf, fragmentSize); 
      if (status != fragmentSize)
	printf ("Recorded a different number of bytes than expected (recorded %d bytes, expected %d)\n", status, fragmentSize);
      printf (".");fflush (stdout);
      
      

      status = write (file, buf, fragmentSize);
      if (status != fragmentSize)
	perror ("Written in file a different number of bytes than expected"); 
      
      cycles ++;
    }
  
  printf("Recording finished.\n");
  free (buf);
  
}





/*=====================================================================*/
/* this function opens an existing file fileName. It reads numBytesToRead bytes from file fileName, and sends it to the soundcard. The play operation is performed in 'fragmentSize' chunks.  If numBytesToRead is 0, it reads forever - the application calling should handle the SIGINT signal to orderly stop the operation.
If an error is found in the configuration of the soundcard, the process is stopped and an error message reported. */
void play (int descSnd, int numBytesToWrite, const char * fileName, int fragmentSize)
{

  int file;
  int cycles = 0;
  int maxCycles = numBytesToWrite / fragmentSize; /* we round down the number of bytes to record to fit with the requested fragmentSize */
  int status;

  /* Creates buffer to store the audio data */
  buf = malloc (fragmentSize); 
  if (buf == NULL) { perror("Could not reserve memory for audio data.\n"); exit (-1); /* very unusual case */ }
  allocatedBuf = true;
  
  /* opens file in read-only mode */
  CALL (file = open (fileName, O_RDONLY), "File could not be opened");

  /* If you need to read from the file and process the audio format, this could be the place */
  
  printf("Playing in blocks of %d bytes... :\n", fragmentSize);
  
  while ( ! (((maxCycles != 0) && (cycles > maxCycles)))) 
    { 
      status = read (file, buf, fragmentSize);
      if (status != fragmentSize)
	break; /* the file finished */
      
      
      status = write (descSnd, buf, fragmentSize); 
      if (status != fragmentSize)
  	printf ("Played a different number of bytes than expected (recorded %d bytes, expected %d)\n", status, fragmentSize);

          
      cycles ++;
    }
  printf("Playing finished.\n");
  free (buf);
};





/*=====================================================================*/
/* This function is not currently used by audioSimple; it is provided as an example of how duplex can be used. */
/* this funcion creates a new file fileName. reads numBytesToProcess bytes from the soundcard, writes again to the soundcard (so simultaneous reading and writing to the soundcard occurs), and also writes the data read to a file fileName. Read and write operations are performed in 'fragmentSize' chunks. 
If an error is found in the configuration of the soundcard, the process is stopped and an error message reported. */
void duplex (int descSnd, int numBytesToProcess, const char * fileName, int fragmentSize)
{
  int file;
  int cycles = 0;
  int maxCycles = numBytesToProcess / fragmentSize; /* we round down the number of bytes to record to fit with the requested fragmentSize */
  int status;
  int ioctl_arg = 0;

  /* Creates buffer to store the audio data */
  buf = malloc (fragmentSize); 
  if (buf == NULL) { perror("Could not reserve memory for audio data.\n"); exit (-1); /* very unusual case */ }
  allocatedBuf = true;
  
  
  /* sets soundcard in duplex mode */
  CALL( ioctl(descSnd, SNDCTL_DSP_SETDUPLEX, &ioctl_arg), "Error setting duplex mode");  

  
  /* opens file for writing */
  CALL (file = open  (fileName, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU), "Error creating file for writing");
  
  printf("Playing and storing in duplex mode, with blocks of %d bytes... :\n", fragmentSize);
  
  while ( (numBytesToProcess == 0) || (cycles < maxCycles)) 
    { 
      status = read (descSnd, buf, fragmentSize); 
      if (status != fragmentSize)
      perror("Error: number of bytes recorded different than expected");
      printf (".");fflush (stdout);
      
      status = write (descSnd, buf, fragmentSize); 
      if (status != fragmentSize)
	      perror("Error: number of bytes played different than expected");
      printf (".");fflush (stdout);
      

      status = write (file, buf, fragmentSize);
      if (status != fragmentSize)
	perror ("Error: number of bytes written to the file different than expected"); 
      
      cycles ++;
    }
  
  printf("Finished duplex mode.\n");
  free (buf);
}

