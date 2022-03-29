#include <netinet/in.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>


#define PORT 11000
  
int main(int argc, char const* argv[])
{
    int len;
    
    // Creamos el socket en el servidor
    int socketD = socket(AF_INET, SOCK_STREAM, 0);
  
    // Creo un string para enviar mensaje al cliente
    char mensaje[255];                       
  
    // Estructura donde defino la direccion del servidor
    struct sockaddr_in servAddr;
  
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(PORT); //Puerto que usamos definido en define
    servAddr.sin_addr.s_addr = INADDR_ANY;
  
    // bindeo el socket a la IP y puerto especificado
    bind(socketD, (struct sockaddr*)&servAddr, sizeof(servAddr));
  
    // Escucho las conexiones (Toma el socket y prepara la estructura)
    listen(socketD, 64);
    
    printf("Esperando conectar con el cliente...");
  
    // Accept = Recibe el socket del cliente (1er llamada bloqueante)
    // Cuando se establece conexion retorna un fhc (un entero)
    int socketCliente = accept(socketD, NULL, NULL);
    
    // Ciclo de solicitud de datos del cliente
    while (1) {
    	// Leemos lo que nos envia el cliente y le respondemos que recibimos el mensaje
        len = read(socketCliente, mensaje, sizeof(mensaje));
        
	// leer devuelve 0 para indicar que el par se ha cerrado
	if(len == 0){
	    break;
	}
			       
        write(STDOUT_FILENO, mensaje, len);
    }
   
    // Cerramos la conexion
    close(socketD);
    close(socketCliente);
  
    return 0;
}
