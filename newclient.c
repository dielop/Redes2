#include <netinet/in.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <arpa/inet.h>

#define PORT 11000
  
int main(int argc, char const* argv[])
{
    int len;
    char mensaje[255];
    //Creamos un socket en el cliente
    int sd = socket(AF_INET, SOCK_STREAM, 0);
  
    //Estructura donde defino la direccion del servidor
    struct sockaddr_in servAddr;
  
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(PORT); //Puerto que usamos definido en define
    servAddr.sin_addr.s_addr = INADDR_ANY;
  
    int connectStatus = connect(sd, (struct sockaddr*)&servAddr, sizeof(servAddr));
    
    //Si la conexion falla tira un error, si no, recibe el mensaje del servidor.
  
    if (connectStatus == -1) {
        printf("Error...\n");
    }
    
    printf("Para cortar la comunicacion envie la letra Q\n");
    
    while(1){
     
        fgets(mensaje, sizeof(mensaje), stdin);
        
        // Escribir datos en el servidor
        write(sd, mensaje, strlen(mensaje)); 
        
        // Leer mensajes del servidor
        len = read(sd, mensaje, sizeof(mensaje));        
        write(STDOUT_FILENO, mensaje, len);

        if(mensaje[0] == 'Q'){
            break;
        }
    }
    
    close(sd);  
    return 0;
}
