//30-12-16, vamos a ver...
#include "conf.h"
#include "confArgs.h"
#include "configureSndcard.h"
#include "circularBuffer.h" 
#include "rtp.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
//#define MAXBUF 256	
#define MAXBUF 65536
#ifndef CALL
#define CALL(v,m) {if ( (v)==-1) {perror (m); printf ("Error Number: %d, %s\n", errno, strerror (errno)); exit (1); }};
#endif

void rellenarCabecera(rtp_hdr_t *rtpPacket, int payload, int n_seq, int timestamp, int identificador_ssrc){

	(*rtpPacket).version = 2;
	(*rtpPacket).m = 0;
	(*rtpPacket).x = 0;
	(*rtpPacket).cc = 0;
	(*rtpPacket).m = 0;
	(*rtpPacket).pt = payload;
	(*rtpPacket).seq = n_seq;
	(*rtpPacket).ts = timestamp;	
	(*rtpPacket).ssrc = identificador_ssrc;	
}

/*Compilar:

gcc -o exec_conf conf.c confArgs.c configureSndcard.c circularBuffer.c

*/

int main(int argc, char * argv[]){
	
	char reserva;
	int operation = 0;
	char * firstIP = (char *)malloc(1);
	char * firstMulticastIP = (char *)malloc(1);	
	int  port = 0;
	int  vol = 0;
	int  packetDuration = 0; 
	int packetDuration_potencia_2 = 0;
	int  verbose = 0;
	int  payload = 0; 
	int  bufferingTime = 0;
	int status = 0;
	char *buf;
	int descSnd = 0;
	struct structSndQuality *dataSndQuality = (struct structSndQuality *)malloc(1);
	int fragmentSize = 0;
	int numberOfZones = 0;
	void * circularBufferCreation = (void *)malloc(1);
	void * puntero_insertar_en_buffer = (void *)malloc(1);
	void * puntero_leer_en_buffer = (void *)malloc(1);
	rtp_hdr_t * cabeceraRTP;
	void * paquete;
	int n_seq=0;	
	int timestamp = 0;
	int identificador_ssrc=0;
	int s, r;
	struct sockaddr_in local, remote, remote_caso_multicast,direccion_envio;
	struct ip_mreq mreq;
	char buffer_socket[MAXBUF]; 
	socklen_t from_len; 
	int tam_pq_recibido=0;
	int mayor_descriptor=0;
    int resultado=0;
	int ultimo_n_seq=0;
	int ultimo_timestamp=0;
	fd_set d_lectura, d_escritura;
	void *ultimo_pq_recibido;
	char * puntero_a_carga_util;
	int incrementoTimeStamp;
	int multicast=0;
    struct timeval tiempo;
	void *posReadBuffer;
	int veces_timer_salta = 0;
	int n_pq=0;
	int n_pq_auxiliar = 0;
	int n_bytes_snd;
	int n_bytes_queue;
	float t_queue;
	//*************************************************************************************************************//
	//SECCIÓN 1****************************************************************************************************//
	//*************************************************************************************************************//
	
	captureArguments (argc, argv, & operation, firstIP, firstMulticastIP, &port, &vol, &packetDuration, &verbose, &payload, &bufferingTime);
	//printValues (operation, firstIP, firstMulticastIP, port, vol, packetDuration, verbose, payload, bufferingTime);

	if (operation == FIRST) {

		printf ("FIRST mode\n");
		fflush(stdout);
		identificador_ssrc=0;

		if(firstMulticastIP[0] == '\0'){ //Unicast
			
			printf("UNICAST\n");
			fflush(stdout);
			bzero(&local, sizeof(local));
			local.sin_family = AF_INET;
			local.sin_port = htons(port); 
			local.sin_addr.s_addr = htonl (INADDR_ANY);
			if (inet_pton(AF_INET, firstIP, &local.sin_addr) < 0) {
				perror("inet_pton");
			    return 1;
			}
			if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			    perror("socket");
			    return 1;
			}
			if (bind(s, (struct sockaddr *)&local, sizeof(local)) < 0) {
			    perror("bind");
			    return 1;
			}
			bzero(&remote, sizeof(remote));
            from_len = sizeof (remote); 

            if ((r = recvfrom(s, buffer_socket, MAXBUF, 0, (struct sockaddr *) &remote, &from_len)) < 0) {
				perror ("recvfrom");
			}

		}else{	//Multicast
			multicast=1;
			printf("MULTICAST!\n");
			fflush(stdout);
			bzero(&local, sizeof(local));
			bzero(&remote_caso_multicast, sizeof(remote_caso_multicast));
			bzero(&direccion_envio, sizeof(direccion_envio));
			local.sin_family = AF_INET;
			local.sin_port = htons(port);			
			if (inet_pton(AF_INET, firstMulticastIP, &local.sin_addr) < 0) {
				perror("inet_pton");
			    return 1;
			}
			if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			    perror("socket");
			    return 1;
			}
			if (bind(s, (struct sockaddr *)&local, sizeof(local)) < 0) {
			    perror("bind");
			    return 1;
			}
			//SUSCRIBIRSE A GRUPO MULTICAST	
			if (inet_pton(AF_INET, firstMulticastIP, &mreq.imr_multiaddr) < 0) {
			    perror("inet_pton");
			    return 1;
			}
			   if (inet_pton(AF_INET, firstMulticastIP, &mreq) < 0) {
			    perror("inet_pton");
			    return 1;
			}
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);
						
			if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
				perror("setsockopt");
				return 1;
			}

            from_len = sizeof (remote); 

            if ((r = recvfrom(s, buffer_socket, MAXBUF, 0, (struct sockaddr *) &direccion_envio, &from_len)) < 0) {
				perror ("recvfrom");
			}
		}

		
		//NOTAS:
		//1-cambiar el tipo de dato de buffer_socket a * void->no creo que sea éste el fallo, porque el número de secuencia y demás se imprime bien
		//2-cambiar el from_len por sizeof(struct sockaddrin) [mirar en lo de nacho]
		//4-hacer pruebas con el código del día 20 que fue cuando le mandé el correo a Alberto, y le dije que con el -y11 funcionaba bien :|

		ultimo_pq_recibido = malloc(r);
        tam_pq_recibido = r;
        cabeceraRTP  = (rtp_hdr_t *)buffer_socket;
        //HACER AQUÍ UN PRINTF DE LO QUE DEVUELVE ESTE PUNTERO CUANDO SE USA EL BUFFER SOCKET o un puntero a éso
       	ultimo_n_seq=cabeceraRTP -> seq;
       	ultimo_timestamp=cabeceraRTP ->ts;

		if (cabeceraRTP ->pt == 100){
			dataSndQuality->format = 8;
			dataSndQuality->channels = 1;
			dataSndQuality->freq = 8000;
		}else if (cabeceraRTP ->pt == 11){
			dataSndQuality->format=16;
			dataSndQuality->channels = 1;
			dataSndQuality->freq = 44100;
		}

		fragmentSize=tam_pq_recibido-sizeof(rtp_hdr_t);
		buf = malloc (fragmentSize); 
		configSndcard (&descSnd, dataSndQuality, &fragmentSize);	
		configVol (dataSndQuality->channels, descSnd , vol);	
		int ioctl_arg = 0;
		CALL( ioctl(descSnd, SNDCTL_DSP_SETDUPLEX, &ioctl_arg), "Error setting duplex mode"); 

		packetDuration_potencia_2 = 1000*fragmentSize / ( (dataSndQuality->channels) * (dataSndQuality->format /8) * (dataSndQuality->freq) ) ;
		numberOfZones = (bufferingTime / packetDuration_potencia_2) + (200 / packetDuration_potencia_2);
		circularBufferCreation = createCircularBuffer (numberOfZones, fragmentSize);

  	} else if (operation == SECOND) {
  		printf ("SECOND mode.\n");
  		fflush(stdout);

		identificador_ssrc=1;
		if (payload == 100){
			dataSndQuality->format = 8;
			dataSndQuality->channels = 1;
			dataSndQuality->freq = 8000;
		}else if (payload == 11){
			dataSndQuality->format=16;
			dataSndQuality->channels = 1;
			dataSndQuality->freq = 44100;
		}
		fragmentSize = packetDuration * (dataSndQuality->channels) * (dataSndQuality->format /8) * (dataSndQuality->freq);
		fragmentSize = fragmentSize/1000; //dividimos entre 1000 para cuadrar unidades
		
		configSndcard (&descSnd, dataSndQuality, &fragmentSize);	
		configVol (dataSndQuality->channels, descSnd , vol);	
		
		int ioctl_arg = 0;
		CALL( ioctl(descSnd, SNDCTL_DSP_SETDUPLEX, &ioctl_arg), "Error setting duplex mode");  
		
		packetDuration_potencia_2 = 1000*fragmentSize / ( (dataSndQuality->channels) * (dataSndQuality->format /8) * (dataSndQuality->freq) ) ;
		
		numberOfZones = (bufferingTime / packetDuration_potencia_2) + (200 / packetDuration_potencia_2);
		circularBufferCreation =  createCircularBuffer (numberOfZones, fragmentSize);
		//Grabación con tarjeta de sonido
		buf = malloc (fragmentSize); 
		if (buf == NULL) { printf("Could not reserve memory for audio data.\n"); exit (-1); }
		status = read (descSnd, buf, fragmentSize); 
		paquete = malloc( status + 12 );


		cabeceraRTP = (rtp_hdr_t *)paquete;

		rellenarCabecera(cabeceraRTP, payload, n_seq, n_seq*incrementoTimeStamp, identificador_ssrc);

		buf = ((char*) paquete) +sizeof(rtp_hdr_t);	//Repasar cómo llego hasta aquí y los pasos anteriores
		n_seq += 1; 

	    /* preparing bind */
	    bzero(&local, sizeof(local));
	    local.sin_family = AF_INET;
	    local.sin_port = htons(port);
	    local.sin_addr.s_addr = htonl (INADDR_ANY);
	  
	    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	      perror("socket");
	      return 1; 
	    }

	    if (bind(s, (struct sockaddr *)&local, sizeof(local)) < 0) {
	      perror("bind");
	      return 1;
	    }

	  	bzero(&remote, sizeof(remote));

	  	remote.sin_family = AF_INET;
	  	remote.sin_port = htons(port);
	  	if (inet_pton(AF_INET, firstIP, &remote.sin_addr) < 0) {
	    	perror("inet_pton");
	    	return 1;
	  	}

		if ( (r = sendto(s, paquete, sizeof(rtp_hdr_t)+fragmentSize,  0, (struct sockaddr *) &remote, sizeof(remote)))<0) {
			perror ("sendto");
	    }
	    if(verbose==1){
	    	printf(".");
        	fflush(stdout);
	    }
	  		
	}
	
	//*************************************************************************************************************//
	//FIN SECCIÓN 1************************************************************************************************//
	//*************************************************************************************************************//

    if(s>descSnd){
    	mayor_descriptor=s;
    }else{
    	mayor_descriptor=descSnd;
    }


	int tam_buffer_a_acumular = bufferingTime/packetDuration_potencia_2;
	int tam_libre_buffer=numberOfZones;


	int comienza_reproduccion = 0;
	ultimo_pq_recibido = malloc(sizeof(rtp_hdr_t) + fragmentSize);
	incrementoTimeStamp=fragmentSize/(dataSndQuality->format /8);
	

    while(1){

    	FD_ZERO(&d_lectura);
		FD_ZERO(&d_escritura);
		FD_SET(s,&d_lectura);
		FD_SET(descSnd,&d_lectura);
		FD_SET(descSnd,&d_escritura);


		//Configuración del timer del select
		CALL(ioctl(descSnd, SNDCTL_DSP_GETODELAY,&n_bytes_snd), "Error con SNDCTL_DSP_GETODELAY");
		n_bytes_queue = n_bytes_snd+n_pq_auxiliar*fragmentSize;
		t_queue = n_bytes_queue / ( (dataSndQuality->channels) * (dataSndQuality->format /8) * (dataSndQuality->freq) ) ;
		t_queue = (float)n_bytes_queue / ( (dataSndQuality->channels) * (dataSndQuality->format /8) * (dataSndQuality->freq) ) ;
		tiempo.tv_sec = (long)(int)t_queue;
		tiempo.tv_usec = (t_queue-tiempo.tv_sec)*1000000;

	    if(n_pq < tam_buffer_a_acumular){	//mientras no alcance el tamaño mínimo de pq's para enviar a la tarjeta de sonido (tam_libre_buffer)

	    	FD_ZERO(&d_escritura);
        	resultado=select(mayor_descriptor+1,&d_lectura,NULL,NULL,NULL);

     	}else{

			/*
			printf("tiempo.tv_sec: %d\n",tiempo.tv_sec);
			printf("tiempo.tv_usec: %d\n",tiempo.tv_usec);
			fflush(stdout);
			*/
     		comienza_reproduccion = 1;

     		if(n_pq_auxiliar > 0){
				FD_SET(descSnd,&d_escritura);
     			resultado=select(mayor_descriptor+1, &d_lectura, &d_escritura,NULL, &tiempo);
     		}else{
     			resultado=select(mayor_descriptor+1, &d_lectura, NULL,NULL, &tiempo);	//¿poner un valor a tiempo concreto para que no se quede siempre enviado?
     		}
     	}

     	if(resultado == -1){
     		perror("El select está devolviendo -1");
     	}else if(resultado ==0 ){	//salta timer

     		if(verbose==1){
	    		printf("t");
        		fflush(stdout);	
	    	}

     		if(veces_timer_salta < 5){
	     		/*cabeceraRTP  = (rtp_hdr_t *)ultimo_pq_recibido;
	     		puntero_a_carga_util=((char *) cabeceraRTP) + sizeof(rtp_hdr_t);
	     		write (descSnd, puntero_a_carga_util, fragmentSize);
	     		veces_timer_salta ++;
	     		if(verbose==1){
		    		printf("x");
	        		fflush(stdout);
		    	}*/
	     	}/*else{
	     		veces_timer_salta = 0;
	     	}*/

     	}else{

				if (FD_ISSET(s,&d_lectura) == 1 ){

					veces_timer_salta = 0;
					if(tam_libre_buffer>0){
						
	    				
			    		from_len = sizeof (remote); 	

						//if ((r = recvfrom(s, buffer_socket, MAXBUF, 0, (struct sockaddr *) &remote, &from_len)) < 0) {
			    		if ((r = recvfrom(s, buffer_socket, MAXBUF, 0, (struct sockaddr *) &remote_caso_multicast, &from_len)) < 0) {

							perror ("recvfrom");
						}

						//Cada vez que me llega un pq por el socket lo almaceno como el ultimo paquete recibido
						memcpy(ultimo_pq_recibido,buffer_socket,r);
			       		tam_pq_recibido = r;
			       		cabeceraRTP  = (rtp_hdr_t *)buffer_socket;
						
						puntero_a_carga_util=((char *) cabeceraRTP) + sizeof(rtp_hdr_t);	//aquí consigo un puntero al payload del paquete

						//Ahora habrá que comprobar lo recibido y meterlo en el buffer
						int ns=cabeceraRTP -> seq;
						//printf("N_SECUENCIA: %d\n",ns);
						//fflush(stdout);	
				       	if (cabeceraRTP -> seq != ultimo_n_seq+1) {
				       		if(cabeceraRTP -> seq < ultimo_n_seq){
				       			if(verbose==1){
		    						printf("d");
	        						fflush(stdout);
		    					}
				       		}else{
				       			if(verbose==1){
		    						printf("s");
	        						fflush(stdout);
		    					}
				       		}
				       	}else{
					       	ultimo_n_seq=cabeceraRTP->seq;
					       	ultimo_timestamp=cabeceraRTP->ts;
					       	/*printf("ULTIMO_N_SEQ: %d, ULTIMO_TIMESTAMP%d\n",ultimo_n_seq,ultimo_timestamp);
							fflush(stdout);*/

				        }


				       	puntero_insertar_en_buffer=pointerToInsertData(circularBufferCreation);
				       	memcpy(puntero_insertar_en_buffer,puntero_a_carga_util,fragmentSize);
				       	if(verbose==1){
							printf("+");
							fflush(stdout);
						}
				       	tam_libre_buffer --;

				       	if(comienza_reproduccion == 0){
				       		n_pq ++;	
				   		}
				       	n_pq_auxiliar++;

					}else{
						perror("El buffer circular se ha llenado");
						fflush(stdout);
					}

					//PRUEBAS--------------------------------------


				/*	write( descSnd, puntero_a_carga_util, fragmentSize);	
					printf("Estoy reproduciendo en el host directamente al recibir\n");
					fflush(stdout);*/
	    			//PRUEBAS--------------------------------------
		
	    		}

    		
				if (FD_ISSET(descSnd,&d_lectura) == 1 ){

	        		paquete=malloc(sizeof(rtp_hdr_t) + fragmentSize);
	        		cabeceraRTP = (rtp_hdr_t *)paquete;
	        		buf=((char*) paquete) + sizeof(rtp_hdr_t);

					rellenarCabecera(cabeceraRTP, payload, n_seq, n_seq*incrementoTimeStamp, identificador_ssrc);

					n_seq += 1;
					read( descSnd, buf, fragmentSize);

					if(multicast==0){
						if ( (r = sendto(s, paquete, sizeof(rtp_hdr_t)+fragmentSize,  0, (struct sockaddr *) &remote, sizeof(remote)))<0) {
							perror ("sendto");
				    	}
			    	}else{
			    		if ( (r = sendto(s, paquete, sizeof(rtp_hdr_t)+fragmentSize,  0, (struct sockaddr *) &direccion_envio, sizeof(direccion_envio)))<0) {
							perror ("sendto");
				    	}
			    	}
					if(verbose==1){
						printf(".");
						fflush(stdout);
					}


			    	//PRUEBAS--------------------------------------

/*
					write( descSnd, buf, fragmentSize);	
					printf("Estoy reproduciendo en el host donde grabo\n");
					fflush(stdout);
					*/
	    			//PRUEBAS--------------------------------------


		    	}

	    	if (FD_ISSET(descSnd,&d_escritura) == 1 && n_pq_auxiliar>0){
		  		void *posReadBuffer;
		  		posReadBuffer = pointerToReadData (circularBufferCreation);
				memcpy(buf, posReadBuffer, fragmentSize);
				if(verbose==1){
					printf("+");
					fflush(stdout);
				}
	    		n_pq_auxiliar--;
		  		tam_libre_buffer ++;
		  		write( descSnd, buf, fragmentSize);	
    		}	

			
     	}


    }

    printf ("HEMOS SALIDO DEL BUCLE");
	
	return 0;

}





