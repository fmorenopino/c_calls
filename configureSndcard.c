/*******************************************************/
/* configureSndcard.c */
/*******************************************************/

#include "configureSndcard.h"

#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/soundcard.h>

#include <string.h> /* strerror */



unsigned int floor_log2(register unsigned int x); /* function to efficiently compute the log2 floor of an integer, i.e. the integer part of the log2 of another integer. Examples: 
floor_log2(31) -> 4
floor_log2(32) -> 5
floor_log2(33) -> 5 */
/* the floor_log2 function is defined at the end of the file */

/* SOUNDCARD CONFIGURATION */
/* We separate (bits, chan number, frequency) configuration from volume configuration */

/*=====================================================================*/
void configSndcard (int *descSnd, struct structSndQuality *datosQosSnd, int *requestedFragmentSize)
 {
 	 
  unsigned int frgmArg, requestedFrgmArg; /* local variables for fragment configuration */
  
  int ioctl_arg;	
  
  if ( (*descSnd) == 0) /* the device has not been opened before */
    {
      /* opens the audio device if it was not opened before */
      CALL ((*descSnd) = open("/dev/dsp", O_RDWR), "Error opening  /dev/dsp - check (*) if file /dev/dsp exists or (*) if other program is using /dev/dsp");
      
      /* CONFIGURING FRAGMENT SIZE */
      /* It sets the fragmen size only if it is opened for the first time. Remember that this operation MUST be done before additional configuration of the sound card */
      
      /* Argument to construct: 0xMMMMSSSS  , being MMMM fragments (MMMM=number of fragments) of size 2^SSSS (SSSS= log2 of the size of the fragments. 
      We set the size of the fragments, and we do not request anything about the number of fragments */    
      requestedFrgmArg = floor_log2 (*requestedFragmentSize); /* floor_log2 is defined at the end of this file. Sets ‘SSSS’ part of fragment argument */ 
      
      if (requestedFrgmArg > 16) { /* too large value, exceeds 65536 fragment size */
      	      perror ("Requested fragment size is too large, exceeds 65536\n"); 
      	      exit (-1);
      }
      requestedFrgmArg = requestedFrgmArg | 0x7fff0000 ; /* this value is used to request the soundcard not to limit the number of fragments available. Sets ‘MMMM’ part of fragment argument */
      
      frgmArg = requestedFrgmArg;
      CALL (ioctl ( (*descSnd), SNDCTL_DSP_SETFRAGMENT, &frgmArg), "Failure when setting the fragment size\n");
      if (frgmArg != requestedFrgmArg) 
      	      { printf ("Fragment size could not be set to the requested value: requested argument was %d, resulting argument is %d\n", requestedFrgmArg, frgmArg); 
      	      exit (-1);}
      
      /* returns configured fragment value, in bytes, so it performs 2^frgmArg by shifting one bit the appropriate number of times */
      *requestedFragmentSize = 1 << frgmArg;  /* returns actual configured value of the fragment size, in bytes*/
    }




  /* WARNING: this must be done in THIS ORDER:  bits, chan number, frequency. There are some sampling frequencies that are not supported for some bit configurations... */ 
  
  /* The soundcard format configured by SNDCTL_DSP_SETFMT indicates if there is any compression in the audio played/recorded, and the number of bits used. There are many formats defined in /usr/include/linux/soundcard.h. To illustrate this, we next copy from soundcard.h some of them: 
  
  #define SNDCTL_DSP_SETFMT               _SIOWR('P',5, int) -- Selects ONE fmt
  #       define AFMT_QUERY               0x00000000      -- Return current fmt 
  #       define AFMT_MU_LAW              0x00000001
  #       define AFMT_A_LAW               0x00000002
  #       define AFMT_IMA_ADPCM           0x00000004
  #       define AFMT_U8                  0x00000008
  #       define AFMT_S16_LE              0x00000010      -- Little endian signed 16
  #       define AFMT_S16_BE              0x00000020      -- Big endian signed 16
  #       define AFMT_S8                  0x00000040
  #       define AFMT_U16_LE              0x00000080      -- Little endian U16 
  #       define AFMT_U16_BE              0x00000100      -- Big endian U16 
  #       define AFMT_MPEG                0x00000200      -- MPEG (2) audio 
  #       define AFMT_AC3         	       0x00000400       Dolby Digital AC3 

  For audioSimple (and in general in this lab) we are interested in uncompressed, linear quantified, formats, either 8 and 16 bits. 
  The format values reserved for this are  AFMT_U8 (decimal value 8) and AFMT_S16_LE (decimal value 16).
  So selecting the appropriate format for our application is setting 8 or 16 in the format value, which is consistent with the value of 'datosQosSnd -> format'*/
  ioctl_arg = (datosQosSnd->format);  
  
  CALL( ioctl((*descSnd), SNDCTL_DSP_SETFMT, &ioctl_arg), "Error in the ioctl which sets the format/bit number"); 
  if (ioctl_arg != (datosQosSnd-> format))
    printf("It was not possible to set the requested format/bit number (you requested %d, the ioctl system call returned %d).\n", datosQosSnd->format, ioctl_arg);

  ioctl_arg = (datosQosSnd->channels);  
  CALL(ioctl((*descSnd), SNDCTL_DSP_CHANNELS, &ioctl_arg), "Error in the ioctl which sets the number of channels");
  if (ioctl_arg != (datosQosSnd->channels))
    printf("It was not possible to set the requested number of channels (you requested %d, the ioctl system call returned %d).\n", datosQosSnd->channels, ioctl_arg);

  ioctl_arg = (datosQosSnd->freq);	   
  CALL (ioctl((*descSnd), SNDCTL_DSP_SPEED, &ioctl_arg), "Error in the ioctl which sets the sampling frequency");
  if (ioctl_arg != (datosQosSnd->freq)) {
  	printf ("You requested %d Hz sampling frequency, the system call returned %d. If the difference is not much, this is not an error", datosQosSnd->freq, ioctl_arg); }
  datosQosSnd -> freq = ioctl_arg;
  

 }




/*=====================================================================*/
/*  For stereo, it configures both channels with the same volume. */
/* returns the actual volume set in the soundcard after performing the operation */
int configVol (int channels, int descSnd, int vol)
{
  int volFinal, volLeft, volRight;


  volFinal = volLeft = volRight = vol;

  if (channels == 2)
    {
      volFinal = (volRight << 8) + volLeft; 
    }


  /* configures the same volume for playing and recording */
  CALL (ioctl (descSnd, MIXER_WRITE (SOUND_MIXER_MIC), &volFinal), "Error when seting volume for MIC.\n");
  CALL (ioctl (descSnd, MIXER_WRITE (SOUND_MIXER_PCM), &volFinal), "Error when seting volume for playing.\n");
  

  /* we assume both channels have been equally configured; we take one to return the resulting volume */
  volLeft = volFinal & 0xff;
  
  return volLeft;
}



/* prints the fragment size actually configured */
void printFragmentSize (int descriptorSnd)
{
	unsigned int frgmArg; /* local variables for fragment configuration */
	
	/* get fragment size */
	 CALL (ioctl (descriptorSnd, SNDCTL_DSP_GETBLKSIZE, &frgmArg), "Error getting fragment size");
	 printf ("Fragment size is %d\n", frgmArg);
	 
	 
}



/* the next functions to obtain the logarithm of an integer in an efficient way are copied http://aggregate.org/MAGIC/ 
The reader is referred to this link if he requires more detail on how they work.
*/

unsigned int
ones32(register unsigned int x)
{
        /* 32-bit recursive reduction using SWAR...
	   but first step is mapping 2-bit values
	   into sum of 2 1-bit values in sneaky way
	*/
        x -= ((x >> 1) & 0x55555555);
        x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
        x = (((x >> 4) + x) & 0x0f0f0f0f);
        x += (x >> 8);
        x += (x >> 16);
        return(x & 0x0000003f);
}


unsigned int
floor_log2(register unsigned int x)
{
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
#ifdef	LOG0UNDEFINED
        return(ones32(x) - 1);
#else
	return(ones32(x >> 1));
#endif
}







