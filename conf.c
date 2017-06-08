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
#define MAXBUF 65536
#ifndef CALL
#define CALL(v,m) {if ( (v)==-1) {perror (m); printf ("Error Number: %d, %s\n", errno, strerror (errno)); exit (1); }};
#endif

void rellenarCabecera(rtp_hdr_t *rtpPacket, int payload, int n_seq, u_int32 timestamp, int identificador_ssrc){

	(*rtpPacket).version = 2;
	(*rtpPacket).m = 0;
	(*rtpPacket).x = 0;
	(*rtpPacket).cc = 0;
	(*rtpPacket).m = 0;
	(*rtpPacket).pt = payload;
	(*rtpPacket).seq = htons(n_seq);
	(*rtpPacket).ts = htonl(timestamp);	
	(*rtpPacket).ssrc = htons(identificador_ssrc);	
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
	char * puntero_a_carga_util_2;
	int incrementoTimeStamp;
	int multicast=0;
    struct timeval tiempo;
	int veces_timer_salta = 0;
	int n_pq=0;
	int n_pq_auxiliar = 0;
	int n_bytes_snd;
	int n_bytes_queue;
	float t_queue;
	int ioctl_arg;
	int tam_buffer_a_acumular;
	int tam_libre_buffer;
	int comienza_reproduccion = 0;
	int descartar_pq = 0;
	int veces_insercion = 0;
	int cont_inserciones = 0;
	void *posReadBuffer;

	//*************************************************************************************************************//
	//SECCIÓN 1****************************************************************************************************//
	//*************************************************************************************************************//
	
	captureArguments (argc, argv, & operation, firstIP, firstMulticastIP, &port, &vol, &packetDuration, &verbose, &payload, &bufferingTime);

	if (operation == FIRST) {

		printf ("FIRST mode\n");
		fflush(stdout);
		identificador_ssrc=0;

		if(firstMulticastIP[0] == '\0'){ //Unicast
			
			printf("Unicast\n");
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
			printf("Multicast\n");
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

		ultimo_pq_recibido = malloc(r);
        tam_pq_recibido = r;
        cabeceraRTP  = (rtp_hdr_t *)buffer_socket;
       	ultimo_n_seq= ntohs(cabeceraRTP -> seq);
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
       	puntero_insertar_en_buffer=pointerToInsertData(circularBufferCreation);
       	puntero_a_carga_util=((char *) cabeceraRTP) + sizeof(rtp_hdr_t);
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
		fragmentSize = fragmentSize/1000;

		configSndcard (&descSnd, dataSndQuality, &fragmentSize);	
		configVol (dataSndQuality->channels, descSnd , vol);	
		ioctl_arg = 0;
		CALL( ioctl(descSnd, SNDCTL_DSP_SETDUPLEX, &ioctl_arg), "Error setting duplex mode");  

		packetDuration_potencia_2 = 1000*fragmentSize / ( (dataSndQuality->channels) * (dataSndQuality->format /8) * (dataSndQuality->freq) ) ;
		numberOfZones = (bufferingTime / packetDuration_potencia_2) + (200 / packetDuration_potencia_2);
		circularBufferCreation =  createCircularBuffer (numberOfZones, fragmentSize);
		buf = malloc (fragmentSize); 
		if (buf == NULL) { 
			printf("No se pudo reservar memoria\n"); exit (-1); 
		}
		status = read (descSnd, buf, fragmentSize); 
		paquete = malloc( status + 12 );
		cabeceraRTP = (rtp_hdr_t *)paquete;
		rellenarCabecera(cabeceraRTP, payload, n_seq, n_seq*incrementoTimeStamp, identificador_ssrc);
		buf = ((char*) paquete) +sizeof(rtp_hdr_t);	
		n_seq += 1; 
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
	//SECCIÓN 2****************************************************************************************************//
	//*************************************************************************************************************//

    if(s>descSnd){
    	mayor_descriptor=s;
    }else{
    	mayor_descriptor=descSnd;
    }

	tam_buffer_a_acumular = bufferingTime/packetDuration_potencia_2;
	tam_libre_buffer=numberOfZones;
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

	    if(n_pq < tam_buffer_a_acumular){	//Mientras no alcance el tamaño mínimo de pq's para enviar a la tarjeta de sonido...
	    	FD_ZERO(&d_escritura);
        	resultado=select(mayor_descriptor+1,&d_lectura,NULL,NULL,NULL);

     	}else{	//Una vez alcanzado el tamaño mínimo debemos tener en cuenta el timer...
     		comienza_reproduccion = 1;

     		if(n_pq_auxiliar > 0){
				FD_SET(descSnd,&d_escritura);
     			resultado=select(mayor_descriptor+1, &d_lectura, &d_escritura,NULL, &tiempo);
     		}else{
				FD_ZERO(&d_escritura);     			
				resultado=select(mayor_descriptor+1, &d_lectura, NULL,NULL, &tiempo);
     		}
     	}

     	if(resultado == -1){
     		perror("El select está devolviendo -1");
     	}else if(resultado ==0 ){
     		if(verbose==1){
	    		printf("t");
        		fflush(stdout);	
	    	}

     		if(veces_timer_salta < 5){
	     		cabeceraRTP  = (rtp_hdr_t *)ultimo_pq_recibido;
	     		puntero_a_carga_util=((char *) cabeceraRTP) + sizeof(rtp_hdr_t);
	     		write (descSnd, puntero_a_carga_util, fragmentSize);
	     		veces_timer_salta ++;
	     		if(verbose==1){
		    		printf("x");
	        		fflush(stdout);
		    	}
	     	}
     	}else{

     			/*********************************************LECTURA SOCKET**********************************************/
     			/*********************************************************************************************************/
     		
				if (FD_ISSET(s,&d_lectura) == 1 ){				

					veces_timer_salta = 0;
					descartar_pq = 0;

					if(tam_libre_buffer>0){
	    				
			    		from_len = sizeof (remote); 	

			    		if ((r = recvfrom(s, buffer_socket, MAXBUF, 0, (struct sockaddr *) &remote_caso_multicast, &from_len)) < 0) {
							perror ("recvfrom");
						}

			       		cabeceraRTP  = (rtp_hdr_t *)buffer_socket;
						puntero_a_carga_util=((char *) cabeceraRTP) + sizeof(rtp_hdr_t);

						int ns=ntohs(cabeceraRTP -> seq);
						int ts_l=ntohl(cabeceraRTP -> ts);

				       	if (ns != ultimo_n_seq+1 && ns!=0) {	//Si el número de secuencia no era el esperado se pueden estar dando dos casos:


				       		if(ns< ultimo_n_seq){	//1. Que el paquete llegue retrasado, en dicho caso lo descartamos y no lo introducimos en el buffer
				       			if(verbose==1){
		    						printf("d");
	        						fflush(stdout);	
		    					}
		    					descartar_pq = 1;

				       		}else{	//2. Que esté llegando un paquete con un número de secuencia mayor del esperado, en cuyo caso tendremos que insertar "veces_insercion" veces el último que llegó
				       			if(verbose==1){
		    						printf("s");
	        						fflush(stdout);
	        					}
        						//Si detectamos que se ha perdido un paquete, copiamos a la tarjeta de sonido el último que nos llegó tantas veces como paquetes se hayan perdido
        						veces_insercion = (ntohs(cabeceraRTP -> seq)) - (ultimo_n_seq) - 1;
        						cont_inserciones = 0;
        						cabeceraRTP  = (rtp_hdr_t *)ultimo_pq_recibido;
						     	puntero_a_carga_util_2=((char *) cabeceraRTP) + sizeof(rtp_hdr_t);
        						while(cont_inserciones < veces_insercion){
									puntero_insertar_en_buffer=pointerToInsertData(circularBufferCreation);
						     		memcpy(puntero_insertar_en_buffer,puntero_a_carga_util_2,fragmentSize);
						     		cont_inserciones ++;
						     		if(verbose==1){
										printf("x");
										fflush(stdout);
									}
							       	tam_libre_buffer --;

							       	if(comienza_reproduccion == 0){
							       		n_pq ++;	
							   		}
						       		n_pq_auxiliar++;
						       		ultimo_n_seq = ns;	
					     		}
				       		}
				       	}else{
					       	ultimo_n_seq=ntohs(cabeceraRTP->seq);
				        }

						if(descartar_pq == 0){
							memcpy(ultimo_pq_recibido,buffer_socket,r);
				       		tam_pq_recibido = r;
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
				       }

					}else{
						perror("El buffer circular se ha llenado");
						fflush(stdout);
					}	
	    		}

     			/*****************************************LECTURA TARJETA SONIDO******************************************/
     			/*********************************************************************************************************/

				if (FD_ISSET(descSnd,&d_lectura) == 1 ){					

	        		paquete=malloc(sizeof(rtp_hdr_t) + fragmentSize);
	        		cabeceraRTP = (rtp_hdr_t *)paquete;
	        		buf=((char*) paquete) + sizeof(rtp_hdr_t);
					rellenarCabecera(cabeceraRTP, payload, n_seq, n_seq*incrementoTimeStamp, identificador_ssrc);

					n_seq += 1;

					/*
					//Código para probar el comportamiento de conf cuando se pierden paquetes//
						if(n_seq == 50){
							n_seq = 52;
						}
					*/
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
		    	}

 			/****************************************ESCRITURA TARJETA SONIDO*****************************************/
 			/*********************************************************************************************************/

	    	if (FD_ISSET(descSnd,&d_escritura) == 1 && n_pq_auxiliar > 0){
		  		
		  		posReadBuffer = pointerToReadData (circularBufferCreation);
				memcpy(buf, posReadBuffer, fragmentSize);
	    		n_pq_auxiliar--;
		  		tam_libre_buffer ++;
		  		write( descSnd, buf, fragmentSize);			  		
    		}	
     	}
    }
	return 0;
}
