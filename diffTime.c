/* 'diffTime.c'
To compile, 

gcc -Wall -o diffTime diffTime.c 


Examples of execution

strace -tt -o trace [...]  
cat trace | grep 'write(5' | ./ diffTime
	shows in the screen the time interval between sucessive writing operations to device descriptor #5
*/


#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>


int main () {

	struct timeval first, second, diff;
	long int hours, minutes;
	char s_first[1000], s_second[1000];
	int result;
	
	// Read the first two lines
	// the format of the data from strace is	 	15:24:26.607645
	// pass hours:minutes... to a 'struct timeval' variable in which seconds, which may be very large (more than 60) 
	result = scanf ("%ld:%ld:%ld.%ld %[^\n]", &hours, &minutes, &(first.tv_sec), &(first.tv_usec), s_first); /* ld to scan long int values written as decimal values */
		
	if (result!= 5) {
		printf ("I expected lines starting with something like '15:24:26.607645 ...'\n"); 
		exit (1); }  // it could not read the 5 expected variables 
	first.tv_sec = first.tv_sec + minutes * 60 + hours * 60*60; 
	
	result = scanf ("%ld:%ld:%ld.%ld %[^\n]", &hours, &minutes, &(second.tv_sec), &(second.tv_usec), s_second);
	

	if (result!= 5) {
		printf ("I expected lines starting with something like '15:24:26.607645 ...'\n");
		exit (1); }
	second.tv_sec = second.tv_sec + minutes * 60 + hours * 60*60;
	
	
	while (1) { // the loop stops when an EOF 
		timersub (&second, &first, &diff);
		printf ("%ld.%6ld:    %s\n", diff.tv_sec, diff.tv_usec, s_first); /* ld: long integer printed as 'decimal' */
		
		// move data from second to first
		first.tv_sec = second.tv_sec; first.tv_usec = second.tv_usec;
		strcpy (s_first, s_second);
		
		// read new data for second
		 
		result = scanf ("%ld:%ld:%ld.%ld %[^\n]", &hours, &minutes, &(second.tv_sec), &(second.tv_usec), s_second);

		if (result!=5) {
			
			if (result == EOF) {
			exit (0); } // this is assumed normal termination of the code
			else {
				printf ("Unexpected format\n");
				exit (1);
				}
		}
				
		
		second.tv_sec = second.tv_sec + minutes * 60 + hours * 60*60;
	}

	
	return 1; /* everything ended fine */
}	
		

