/*******************************************************/
/* circularBuffer.h */
/*******************************************************/

/* Declaration of functions for managing a circular buffer  */



/*  
Only use in single-process code, such as one managing concurrency by select. 

It allows many buffers existing at the same time.

The pointer returned by executing  createCircularBuffer is the pointer that identifies a particular circular buffer, and is the one required for the rest of the calls

From now on, pointerToInsertData request an available block (for example, to write a data block received from the network).
The opposite operation, accessing to a block, is performed with pointerToReadData
Finally, destroyBuffer frees the memory allocated for the buffer. */

/* ---------------------------------------------------------*/

/* returns a pointer to the memory used to contain the circular buffer (including some memory space used for control). This function ALLOCATES the memory used by the circular buffer. numberOfZones is the number of blocks for the buffer. blockSize is the size in bytes of the buffer */
void * createCircularBuffer (int numberOfZones, int blockSize);

/*
 * n zonas: 
 * 	-k90 : quiero acumular 90ms de datos en el instante en que vaya a comenzar la reproducción: dividos ese 90 entre el tamaño de paquetes (en ms también), y obtendo el número de paquetes
 *  	que voy a esperar (éso es el número de zonas)->debo además empezar con cierto hueco de más (200ms de hueco, que son (200ms/tamaño pq)) por si llegan pq's muy rápido
 * 
 * tamaño de cada bloque: en el buffer sólo metemos el payload (NO la cabecera)->depende del parámetro '-l' (¿fragment size?)-> el tamaño de bloque es el número de bytes
 * que finalmente "cogemos" (después de hacer el downgrade a la potencia de 2 inferior)
 * 
 * 


/* returns a pointer to the first ("empty") available block to write on it, or NULL if there are no blocks (be sure that this case is considered in your code!)  */
void * pointerToInsertData (void * buffer);


/* returns a pointer to the first available block to be read, or NULL if there are no blocks (be sure that this case is considered in your code!)  */
void * pointerToReadData (void * buffer);

/* frees memory of the buffer. Must be executed before exiting from the process */
void destroyBuffer (void *buffer);
