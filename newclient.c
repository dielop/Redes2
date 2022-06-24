#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <err.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>

#define BUFSIZE 512

// Recibe la respuesta del servidor -sd: socket -code:3 numeros para checkear -text: si recibe por parametro se copia la resp.
bool recv_msg(int, int, char *);

// Envia msj al servidor -sd: socket -operation: comando de 4 letras -param: parametros.
void send_msg(int, char *, char *);

// Lee lo ingresado por teclado.
char * read_input();

// Realiza la autenticacion del usuario.
void authenticate(int);

// Chequeo el puerto
bool port(int, char *, int);

// Operacion de solicitud de archivo.
void get(int, char *);

// Creo un directorio.
void mkdir(int, char *);

// Elimino un directorio.
void rmdirec (int, char *);

// Cambio de directorio.
void cd(int, char *);

// listar archivos de un directorio
void listar(int, char *);

// Comando quit para terminar la conexion.
void quit(int);

// Gestion de las operaciones a realizar.
void operate(int);

// Chequeo direccion de puerto
bool direccion_puerto(char *);

/**
 * Run with
 *         ./myftp <SERVER_IP> <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {
    char buffer[BUFSIZE];
    int sd, puerto;
    struct sockaddr_in addr;

    // Chequeo los argumentos y errores
    if(argc!=3){
        printf("Error en cantidad de argumentos\n");
    }
    
    if(!direccion_puerto(argv[2])){
        printf("Error en puerto\n");
    }

    // Creo un socket y chequeo errores
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd < 0){
      printf("Error en la creacion del socket\n");
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

    // cierro el socket
    close(sd);

    return 0;
}

bool recv_msg(int sd, int code, char *text) {
    char buffer[BUFSIZE], message[BUFSIZE];
    int recv_s, recv_code;

    // recibo respuesta
    recv_s = read(sd, buffer, BUFSIZE);

    // error checking
    if(recv_s < 0){
    	printf("Error al recibir datos\n");
    }
    
    if(recv_s == 0){ 
       printf("conexion cerrada por el host\n");
    }

    // parseo el codigo y la respuesta del mensaje
    sscanf(buffer, "%d %[^\r\n]\r\n", &recv_code, message);
    printf("%d %s\n", recv_code, message);
    
    // copia opcional de parametros
    if(text) strcpy(text, message);
    
    // boolean: testeo el codigo
    return (code == recv_code) ? true : false;
}

void send_msg(int sd, char *operation, char *param) {
    char buffer[BUFSIZE] = "";

    // formateo comando
    if (param != NULL){
        sprintf(buffer, "%s %s\r\n", operation, param);
    }
    else{
        sprintf(buffer, "%s\r\n", operation);
    }

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

    // pregunto por el usuario
    printf("username: ");
    input = read_input();

    // envio el comando al server
    send_msg(sd, "USER", input);

    // libero memoria
    free(input);

    // espero a recibir la contrasenia y chequeo errores
    if(!recv_msg(sd, 331, NULL)){
        printf("Usuario incorrecto\n");
    }

    // pregunto por la contrasenia
    printf("password: ");
    input = read_input();

    // envio el comando al server
    send_msg(sd, "PASS", input);

    // libero memoria
    free(input);

    // espero por la respuesta y chequeo errores
    if(recv_msg(sd, 530, NULL)){
        printf("Login incorrecto/error de autenticacion\n");
    }
    
    return;
}

bool port(int sd, char* ip, int puerto){
    char buffer[BUFSIZE], *port_param, *aux;
    int i;
    
    port_param = (char*)malloc(30*sizeof(char)); // Reservo memoria
    aux = (char*)malloc(8*sizeof(char)); // Reservo memoria

    strcpy(port_param, ip);

    i=0;
    while(*(port_param+i) !='\0'){
        if (*(port_param+i) == '.') *(port_param+i) = ',';
        i++;
    }

    sprintf(aux, ",%d,%d",puerto/256, puerto%256);
    strcat(port_param, aux);

	// Envio parametro al server
    send_msg(sd, "PORT", port_param);

	// Libero memoria
    free(port_param);
    free(aux);

    // Chequeo la respuesta y retorno
    return recv_msg(sd, 200, buffer);
}

void mkdir(int sd, char *file_name) {
	char buffer[BUFSIZE], *ip;
    long f_size, recv_s, r_size = BUFSIZE;
    FILE *file;
    int sock1, puerto;
    struct sockaddr_in addr,addr2;
    socklen_t addr2_len = sizeof(addr2);
    socklen_t addr_len = sizeof(addr);

    ip = (char*)malloc(13*sizeof(char)); // Reservo memoria
    
    getsockname(sd, (struct sockaddr *) &addr, &addr_len);
    ip = inet_ntoa(addr.sin_addr);
    puerto = rand()%60000+1024; // Genero un puerto aleatorio mayor a los priv.

    if(!port(sd, ip, puerto)) {
       printf("Respuesta del server invalida\n");
       return;
    }

    // Escucho el canal de datos
    sock1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock1 < 0){
    	printf("No se pudo crear el socket\n");
    }
    
    addr2.sin_family = AF_INET;
    addr2.sin_addr.s_addr = INADDR_ANY;
    addr2.sin_port = htons(puerto);
    
    if (bind(sock1, (struct sockaddr *) &addr2, sizeof(addr2)) < 0){
    	printf("No se pudo bindear\n");
    }
    
    if (listen(sock1,1) < 0){
    	printf("Error en el canal de datos\n");
    }
    
    // envio el comando MKDIR al server
    send_msg(sd, "MKDIR", file_name);
    
    // Chequeo la respuesta
    if(!recv_msg(sd, 299, buffer)) {
       close(sock1);
       return;
    }
      
    // recibo el ok del server
    if(!recv_msg(sd, 226, NULL)){
       printf("MKDIR no termino de forma correcta\n");
	}
	
    // cierro el socket
    close(sock1);

    return;
}

void rmdirec (int sd, char *file_name) {
    char buffer[BUFSIZE], *ip;
    long f_size, recv_s, r_size = BUFSIZE;
    FILE *file;
    int sock1, puerto;
    struct sockaddr_in addr,addr2;
    socklen_t addr2_len = sizeof(addr2);
    socklen_t addr_len = sizeof(addr);

    ip = (char*)malloc(13*sizeof(char)); // Reservo memoria
    
    getsockname(sd, (struct sockaddr *) &addr, &addr_len);
    ip = inet_ntoa(addr.sin_addr);
    puerto = rand()%60000+1024;

    if(!port(sd, ip, puerto)) {
       printf("Respuesta del server invalida\n");
       return;
    }

    // Escucho el canal de datos
    sock1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock1 < 0){
    	printf("No se pudo crear el socket\n");
    }
    
    addr2.sin_family = AF_INET;
    addr2.sin_addr.s_addr = INADDR_ANY;
    addr2.sin_port = htons(puerto);
    
    if (bind(sock1, (struct sockaddr *) &addr2, sizeof(addr2)) < 0){
    	printf("No se pudo bindear\n");
    }
    
    if (listen(sock1,1) < 0){
    	printf("Error en el canal de datos\n");
    }
    
    // envio el comando RMDIREC al server
    send_msg(sd, "RMDIREC", file_name);
    
    // Chequeo la respuesta
    if(!recv_msg(sd, 299, buffer)) {
       close(sock1);
       return;
    }
    
    // recibo el ok del server
    if(!recv_msg(sd, 226, NULL)){
    	printf("RMDIREC no termino de forma correcta\n");
    }
	
    // cierro el socket
    close(sock1);

    return;
}

void cd(int sd, char *file_name){
    char buffer[BUFSIZE], *ip;
    long f_size, recv_s, r_size = BUFSIZE;
    FILE *file;
    int sock1, puerto;
    struct sockaddr_in addr,addr2;
    socklen_t addr2_len = sizeof(addr2);
    socklen_t addr_len = sizeof(addr);

    ip = (char*)malloc(13*sizeof(char)); // Reservo memoria
    
    getsockname(sd, (struct sockaddr *) &addr, &addr_len);
    ip = inet_ntoa(addr.sin_addr);
    puerto = rand()%60000+1024;

    if(!port(sd, ip, puerto)) {
       printf("Respuesta del server invalida\n");
       return;
    }

    // Escucho el canal de datos
    sock1 = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock1 < 0){
    	printf("No se pudo crear el socket\n");
    }
    
    addr2.sin_family = AF_INET;
    addr2.sin_addr.s_addr = INADDR_ANY;
    addr2.sin_port = htons(puerto);
    
    if (bind(sock1, (struct sockaddr *) &addr2, sizeof(addr2)) < 0){
    	printf("No se pudo bindear\n");
    }
    
    if (listen(sock1,1) < 0){
    	printf("Error en el canal de datos\n");
    }
    
    // Envio el comando CD al server
    send_msg(sd, "CD", file_name);
    
    // Chequeo la respuesta
    if(!recv_msg(sd, 299, buffer)) {
       close(sock1);
       return;
    }
    
    // recibo el ok del server
    if(!recv_msg(sd, 226, NULL)){
       printf("CD no termino de forma correcta\n");
    }

    // cierro el socket
    close(sock1);

    return;
}

void listar(int sd, char *file_name){
    char buffer[BUFSIZE], *ip;
    long f_size, recv_s, r_size = BUFSIZE;
    FILE *file;
    int sock1, puerto;
    struct sockaddr_in addr,addr2;
    socklen_t addr2_len = sizeof(addr2);
    socklen_t addr_len = sizeof(addr);

    ip = (char*)malloc(13*sizeof(char)); // Reservo memoria
    
    getsockname(sd, (struct sockaddr *) &addr, &addr_len);
    ip = inet_ntoa(addr.sin_addr);
    puerto = rand()%60000+1024;

    if(!port(sd, ip, puerto)) {
       printf("Respuesta del server invalida\n");
       return;
    }

    // Escucho el canal de datos
    sock1 = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock1 < 0){
    	printf("No se pudo crear el socket\n");
    }
    
    addr2.sin_family = AF_INET;
    addr2.sin_addr.s_addr = INADDR_ANY;
    addr2.sin_port = htons(puerto);
    
    if (bind(sock1, (struct sockaddr *) &addr2, sizeof(addr2)) < 0){
    	printf("No se pudo bindear\n");
    }
    
    if (listen(sock1,1) < 0){
    	printf("Error en el canal de datos\n");
    }
    
    // Envio el comando CD al server
    send_msg(sd, "LISTAR", file_name);
    
    // Chequeo la respuesta
    if(!recv_msg(sd, 299, buffer)) {
       close(sock1);
       return;
    }
    
    // recibo el ok del server
    if(!recv_msg(sd, 226, NULL)){
       printf("LISTAR no termino de forma correcta\n");
    }

    // cierro el socket
    close(sock1);

    return;
}

void get(int sd, char *file_name) {
    char buffer[BUFSIZE], *ip;
    long f_size, recv_s, r_size = BUFSIZE;
    FILE *file;
    int sock1, sock2, puerto;
    struct sockaddr_in addr, addr2;
    socklen_t addr_len = sizeof(addr);
    socklen_t addr2_len = sizeof(addr2);

    ip = (char*)malloc(13*sizeof(char)); // Reservo memoria

    getsockname(sd, (struct sockaddr *) &addr, &addr_len);
    ip = inet_ntoa(addr.sin_addr);
    puerto = rand()%60000+1024;

    if(!port(sd, ip, puerto)) {
       printf("Respuesta del server invalida\n");
       return;
    }

    // Escucho el canal de datos
    sock1 = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock1 < 0){
    	printf("No se pudo crear el socket\n");
    	return;
    }
    
    addr2.sin_family = AF_INET;
    addr2.sin_addr.s_addr = INADDR_ANY;
    addr2.sin_port = htons(puerto);
    
    if (bind(sock1, (struct sockaddr *) &addr2, sizeof(addr2)) < 0){
    	printf("No se pudo bindear\n");
    	return;
    }
    
    if (listen(sock1,1) < 0){
    	printf("Error en el canal de datos\n");
    	return;
    }

    // Envio el comando RETR al server
    send_msg(sd, "RETR", file_name);
    
    // Chequeo la respuesta
    if(!recv_msg(sd, 299, buffer)) {
       close(sock1);
       return;
    }

    // Acepto la nueva conexion
    sock2 = accept(sock1, (struct sockaddr*)&addr2, &addr2_len);
    if (sock2 < 0) {
       printf("Error en la aceptacion de la nueva conexion\n");
       return;
    }

    // Parseo el tamanio del archivo
    // "File %s size %ld bytes"
    sscanf(buffer, "File %*s size %ld bytes", &f_size);

    // Abro el archivo para escribir
    file = fopen(file_name, "w");

    // Recibo el archivo
    while(true) {
    
       if (f_size < BUFSIZE){
       	   r_size = f_size;
       }
       
       recv_s = read(sock2, buffer, r_size);
       
       if(recv_s < 0){
          printf("Se recibio un error\n");
          return;
       }
       
       fwrite(buffer, 1, r_size, file);
       if (f_size < BUFSIZE) break;
       f_size = f_size - BUFSIZE;
    }

    // Cierro el canal de datos
    close(sock2);

    // Cierro el archivo
    fclose(file);
	
    // recibo el ok del server
    if(!recv_msg(sd, 226, NULL)){
       printf("GET no termino de forma correcta\n");
       return;
    }
	
    // cierro el socket
    close(sock1);

    return;
}

void operate(int sd){
    char *input, *op, *param;
    
    while (true) {
        printf("Operation: ");
        input = read_input();
        if (input == NULL){
            continue; //Si es nulo continuo
        }
        
        op = strtok(input, " ");
 
        if (strcmp(op, "get") == 0) {
            param = strtok(NULL, " ");
            get(sd, param);  
        }else if (strcmp (op, "mkdir") == 0) {
			param = strtok(NULL, " ");
			mkdir(sd, param);
        }else if (strcmp (op, "rmdirec") == 0) {
    		param = strtok(NULL, " ");
    		rmdirec(sd, param);
    	}else if (strcmp (op, "cd") == 0) {
    		param = strtok(NULL, " ");
    		cd(sd, param);
    	}else if (strcmp (op, "listar") == 0) {
    		param = strtok(NULL, " ");
    		listar(sd, param);
        }else if (strcmp(op, "quit") == 0) {
        	quit(sd);
            break;
        }else{
            printf("Comando inesperado\n"); 
        }
    }
    
    free(input);
}

bool direccion_puerto(char *string){
    bool verificacion = true;
    int i=0;
    
    while(*(string+i)!='\0'){
        if(!isdigit(*(string+i))){
          verificacion = false;
        }
        i++;
    }
    if(atoi(string)<0||atoi(string)>65535){
    	verificacion = false;
    }
    return verificacion;
}


void quit(int sd){
    // Enviar comando QUIT del cliente
    send_msg(sd, "QUIT", NULL);
    // recibir respuesta del servidor
    if(!recv_msg(sd, 221, NULL)){
        printf("Desconexion incorrecta");
    }
    return;
}