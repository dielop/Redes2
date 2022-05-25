#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <err.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define BUFSIZE 512

//Recibe la respuesta del servidor -sd: socket -code:3 numeros para checkear -text: si recibe por parametro se copia la resp.
bool recv_msg(int, int, char *); 

// Envia msj al servidor -sd: socket -operation: comando de 4 letras -param: parametros.
void send_msg(int, char *, char *);
 
 // Lee lo ingresado por teclado.
char * read_input();

// Realiza la autenticacion del usuario.
void authenticate(int);

bool direccion_IP(char *);

// Verifico que el puerto ingresado sea valido (Digitos y rango).
bool direccion_puerto(char *);

// Comando quit para terminar la conexion.
void quit(int);

/**
 * function: operation get
 * sd: socket descriptor
 * file_name: file name to get from the server
 **/
void get(int sd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    FILE *file;
    int sock1, sock2, puerto; //socket para transferencias
    char *ip;
    ip = (char*)malloc(13*sizeof(char)); //Reservo espacio para la ip
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);//Declaramos una variable que tenga la longitud de la estructura

    puerto = rand()%60000+1024; //Genero un puerto aleatorio mayor a los priv.
    
    // Escuchamos en el canal de datos
    sock1 = socket(AF_INET, SOCK_STREAM, 0);
    if(sock1 < 0){
      printf("Error en la creacion del socket");
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(puerto);
    // send the RETR command to the server
    send_msg(sd, "RETR", file_name);
    
    // Bind para el canal de transferencias de archivos
    if (bind(sock1, (struct sockaddr *) &addr, sizeof(addr)) < 0){
    	printf("No se pudo hacer el bind con el puerto\n");
    }
    // Hago el listen para aceptar al servidor
    if (listen(sock1,1) < 0) {
    	printf("Error en el canal de datos\n");
    }
    
    // check for the response
    if(!recv_msg(sock1, 299, buffer)) {
       close(sock1);
       return;
    }
    // parsing the file size from the answer received
    // "File %s size %ld bytes"
    sscanf(buffer, "File %*s size %d bytes", &f_size);

    // open the file to write
    file = fopen(file_name, "w");

    //receive the file
     while(true) {
       if (f_size < BUFSIZE){
        r_size = f_size;
       }
       
       recv_s = read(sock1, buffer, r_size);
       
       if(recv_s < 0){
        printf("Error\n");
       }
       
       fwrite(buffer, 1, r_size, file);
       
       if (f_size < BUFSIZE){
        break;
       }
       
       f_size = f_size - BUFSIZE;
    }
    
    // Cerrar el canal de datos 
    close(sock1);

    // close the file
    fclose(file);

    // receive the OK from the server
    if(!recv_msg(sd, 226, NULL)){
     printf("La transferencia no se completo correctamente\n");
    }
    
    return;
}


/**
 * function: make all operations (get|quit)
 * sd: socket descriptor
 **/
void operate(int sd) {
    char *input, *op, *param;

    while (true) {
        printf("Operation: ");
        input = read_input();
        if (input == NULL)
            continue; // avoid empty input
        op = strtok(input, " ");
        // free(input);
        if (strcmp(op, "get") == 0) {
            param = strtok(NULL, " ");
            get(sd, param);
        }
        else if (strcmp(op, "quit") == 0) {
            quit(sd);
            break;
        }
        else {
            // new operations in the future
            printf("TODO: unexpected command\n");
        }
        free(input);
    }
    free(input);
}


/**
 * Corro el cliente con la ip y el puerto donde abro el servidor.
 *         ./myftp <SERVER_IP> <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {
    char buffer[BUFSIZE];
    int sd, puerto;
    struct sockaddr_in addr;

    // Chequeo los argumentos y errores
    if(argc!=3){
        printf("Error en cantidad de argumentos");
    }
    if(!direccion_IP(argv[1])){
        printf("Error en ip");
    }
    if(!direccion_puerto(argv[2])){
        printf("Error en puerto");
    }

    // Creo un socket y chequeo errores
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd < 0){
      printf("Error en la creacion del socket");
    }

    puerto = atoi(argv[2]); // Asignamos el puerto a la variable puerto
    // Obtenemos datos del servidor
    addr.sin_family = AF_INET;
    addr.sin_port = htons(puerto); 
    addr.sin_addr.s_addr = inet_addr(argv[1]);//Direccion

    // Conectamos y chequeamos errores
    if(connect(sd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        printf("Error, no se pudo conectar con el servidor.\n");
    }
    
    int clientesz = sizeof(addr);
    getsockname(sd, (struct sockaddr*)&addr, &clientesz);
    printf("Mi puerto es: %d\n", ntohs(addr.sin_port));
         
    // Leemos el mensaje y procedemos con la autenticacion
    if(recv_msg(sd, 220, NULL)){
        authenticate(sd);
        operate(sd);
    }else{
        printf("No se reciben datos del servidor\n");
    }

    // close socket
    close(sd);

    return 0;
}

bool recv_msg(int sd, int code, char *text) {
    char buffer[BUFSIZE], message[BUFSIZE];
    int recv_s, recv_code;

    // receive the answer
    recv_s = read(sd, buffer, BUFSIZE);

    // error checking
    if(recv_s < 0){ 
     printf("Error al recibir datos");
    }
    if(recv_s == 0){ 
     printf("conexion cerrada por el host");
    }
    // parsing the code and message receive from the answer
    sscanf(buffer, "%d %[^\r\n]\r\n", &recv_code, message);
    printf("%d %s\n", recv_code, message);
    // optional copy of parameters
    if(text) strcpy(text, message);
    // boolean test for the code
    return (code == recv_code) ? true : false;
}

void send_msg(int sd, char *operation, char *param) {
    char buffer[BUFSIZE] = "";

    // command formating
    if (param != NULL)
        sprintf(buffer, "%s %s\r\n", operation, param);
    else
        sprintf(buffer, "%s\r\n", operation);

    // Escribe el comando y chequea errores
    write(sd, buffer, strlen(buffer)+1);
    return;
}

char * read_input() {
    char *input = malloc(BUFSIZE);
    if (fgets(input, BUFSIZE, stdin)) {
        return strtok(input, "\n");
    }
    return NULL;
}

void authenticate(int sd) {
    char *input;
    //char desc[100];
    //int code;

    // ask for user
    printf("username: ");
    input = read_input();

    // send the command to the server
    send_msg(sd, "USER", input);

    // relese memory
    free(input);

    // wait to receive password requirement and check for errors
    if(!recv_msg(sd, 331, NULL)){
        printf("Usuario incorrecto\n");
    }

    // ask for password
    printf("passwd: ");
    input = read_input();

    // send the command to the server
    send_msg(sd, "PASS", input);

    // release memory
    free(input);

    // wait for answer and process it and check for errors
    if(recv_msg(sd, 530, NULL))
        printf("Login incorrecto/error de autenticacion\n");
    return;    
}

bool direccion_IP(char *string){
    char *token;
    bool verificacion = true;
    int contador=0,i;
    token = (char *) malloc(strlen(string)*sizeof(char));
    strcpy(token, string);
    token = strtok(token,".");

    while(token!=NULL){
        contador++;
        i=0;
        while(*(token+i)!='\0'){
            if(!isdigit(*(token+i))) verificacion = false;
            i++;
        }
        if(atoi(token)<0||atoi(token)>255) verificacion = false;
        token=strtok(NULL,".");
    }
    if(contador!=4) verificacion = false;
    free(token);

    return verificacion;
}

bool direccion_puerto(char *string){
    bool verificacion = true;
    int i=0;
    while(*(string+i)!='\0'){
        if(!isdigit(*(string+i))) verificacion = false;
        i++;
    }
    if(atoi(string)<0||atoi(string)>65535) verificacion = false;
    return verificacion;
}

void quit(int sd) {
     // Enviar comando QUIT del cliente
    send_msg(sd, "QUIT", NULL);
    // recibir respuesta del servidor
    if(!recv_msg(sd, 221, NULL))
        printf("Desconexion incorrecta");
    return;
}