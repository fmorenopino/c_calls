/*--------------------------------------------------------------------------
 * FICHERO:     receptor_udp.c
 * DESCRIPCION: espera un datagrama en el puerto 4950 y muestra el contenido
 * SINTAXIS:    receptor_udp
 *--------------------------------------------------------------------------
 */

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

#define PUERTO 4950 /* Puerto de conexión */
#define TAM_BUFFER 100 /* Tamaño de buffer */

int main(int argc, char ** argv) {
  int sd; /* Descriptor de socket */
  int num_bytes; /* Número de bytes recibidos */
  struct sockaddr_in emisor_addr; /* Dirección del emisor */
  struct sockaddr_in receptor_addr; /* Dirección del receptor */
  socklen_t addr_len; /* Longitud de la dirección del emisor */
  char buffer[TAM_BUFFER];

  /* Creamos el socket UDP */
  sd = socket(PF_INET, SOCK_DGRAM, 0);
  if (sd < 0) {
    perror("socket");
    exit(1);
  }

  /* Inicializamos la estructura con la dirección del receptor */
  memset(&receptor_addr, 0, sizeof(receptor_addr));
  receptor_addr.sin_family = AF_INET;
  receptor_addr.sin_port = htons(PUERTO);
  receptor_addr.sin_addr.s_addr = INADDR_ANY;
  /* Escucha por cualquier interfaz de red activa del host (cualquier
     tarjeta con IP asignada) */
 
  /* Vincula el socket a la dirección especificada */
  if (bind(sd, (struct sockaddr *) &receptor_addr, sizeof(receptor_addr)) < 0) {
    perror("bind");
    exit(1);
  }
  
  /* Recibimos el datagrama */
  addr_len = sizeof(emisor_addr);
  num_bytes = recvfrom(sd, buffer, TAM_BUFFER, 0, (struct sockaddr *) &emisor_addr, (socklen_t *) &addr_len);
  if (num_bytes < 0) {
    perror("recvfrom");
    exit(1);
  }
  
  printf("Datagrama recibido de %s\n", inet_ntoa(emisor_addr.sin_addr));
  printf("Longitud del datagrama: %d bytes\n", num_bytes);
  buffer[num_bytes] = 0;
  printf("Contenido del datagrama: %s\n", buffer);
  
  close(sd);
  return 0;
}
