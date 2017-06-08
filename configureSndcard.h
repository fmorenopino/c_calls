/*******************************************************/
/* configureSndcard.h */
/*******************************************************/


#ifndef SOUND_H
#define SOUND_H




#ifndef CALL
/* Macro to process system calls that return 1 in case of error */
#define CALL(v,m) {if ( (v)==-1) {perror (m); printf ("Error Number: %d, %s\n", errno, strerror (errno)); exit (1); }};
#endif


/*=====================================================================================*/
/* TYPES */
struct structSndQuality
{
  int format; /* the number which defines the way audio data is coded, according to the soundcard specification - PCM with X bits... */
  int channels; /* 1 mono; 2 stereo */
  int freq; /* sampling frequency in Hz*/
};



/*=====================================================================================*/
/* DEFINITION OF FUNCTIONS */
/* This function changes the configuration for the descSnd provided to the parameters stated in dataSndQuality. 
If an error is found in the configuration of the soundcard, the process is stopped and an error message reported */ 
void configSndcard (int *descSnd, /* If descSnd equals to 0 when calling the function, configSnd creates and initializes a new 			
			sound descriptor, which is returned in (*descSnd) */ 
	struct structSndQuality *dataSndQuality,  /* here the requested quality for the soundcard is passed. 
			The dataSndQuality->freq field is updated with the real value of the frequency configured in the sound card (which may be different than requested). You should check this returned value, because it could happen that your computations are wrong! */
	int *fragmentSize /* This value is is only considered if the sound descriptor is created inside the function, i.e. if the 
			initial value of descSnd was 0. In this case, when calling this function this value contains the REQUESTED fragment size in bytes - this value does not need to be a power of 2. Internally, the function obtains the power-of-two value immediately below the provided number. The function returns this value.
			 */
	);

/* configures volume for descSnd. 
If it is stereo, it configures both channels; otherwise, only the single channel in use. 
The function returns the volume actually configured in the device after performing the operation (could be different than reported).
If an error is found in the configuration of the soundcard, the process is stopped and an error message reported. */
int configVol (int stereo, int descSnd, int vol); /* */

/* To call AFTER soundcard configuration, informs about some of the results of soundcard conf:  */
void printFragmentSize (int descriptorSnd);

#endif /* SOUND_H */


