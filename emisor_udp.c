/*--------------------------------------------------------------------------
 * FICHERO:     emisor_udp.c
 * DESCRIPCION: envía un datagrama al puerto 4950
 * SINTAXIS:    emisor_udp direccion_ip mensaje
 *              Si el mensaje contiene espacios, ponerlo entre comillas
 *--------------------------------------------------------------------------
 */

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

#define PUERTO 4950 /* Puerto de conexión */

int main(int argc, char ** argv) {
  int sd; /* Descriptor de socket */
  int num_bytes; /* Número de bytes enviados */
  struct sockaddr_in receptor_addr; /* Dirección del receptor */
  uint32_t ip_receptor; /* IP del receptor */

  /* Obtenemos la dirección IP del receptor */
  if (argc != 3) {
    fprintf(stderr, "Uso: emisor_udp direccion_ip mensaje\n");
    exit(1);
  }
  
  /* Obtenemos la dirección IP del receptor */
  ip_receptor = inet_addr(argv[1]);
  if (ip_receptor == -1) {
    perror("inet_addr");
    exit(1);
  }
  
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
  memcpy(&receptor_addr.sin_addr, &ip_receptor, 4);
 
  num_bytes = sendto(sd, argv[2], strlen(argv[2]),0, (struct sockaddr *) &receptor_addr, sizeof(receptor_addr));
  if (num_bytes < 0) {
    perror("sendto");
    exit(1);
  }
  
  printf("Enviados %d bytes a %s\n", num_bytes,inet_ntoa(receptor_addr.sin_addr));
 
  close(sd);
  
  return 0;
}
