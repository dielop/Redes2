#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <err.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include<ctype.h>

#define BUFSIZE 512

/**
 * function: receive and analize the answer from the server
 * sd: socket descriptor
 * code: three leter numerical code to check if received
 * text: normally NULL but if a pointer if received as parameter
 *       then a copy of the optional message from the response
 *       is copied
 * return: result of code checking
 **/
/*bool recv_msg(int sd, int code, char *text) {
    char buffer[BUFSIZE], message[BUFSIZE];
    int recv_s, recv_code;

    // receive the answer


    // error checking
    if (recv_s < 0) warn("error receiving data");
    if (recv_s == 0) errx(1, "connection closed by host");

    // parsing the code and message receive from the answer
    sscanf(buffer, "%d %[^\r\n]\r\n", &recv_code, message);
    printf("%d %s\n", recv_code, message);
    // optional copy of parameters
    if(text) strcpy(text, message);
    // boolean test for the code
    return (code == recv_code) ? true : false;
}*/

/**
 * function: send command formated to the server
 * sd: socket descriptor
 * operation: four letters command
 * param: command parameters
 **/
/*void send_msg(int sd, char *operation, char *param) {
    char buffer[BUFSIZE] = "";

    // command formating
    if (param != NULL)
        sprintf(buffer, "%s %s\r\n", operation, param);
    else
        sprintf(buffer, "%s\r\n", operation);

    // send command and check for errors

}*/

/**
 * function: simple input from keyboard
 * return: input without ENTER key
 **/
/*char * read_input() {
    char *input = malloc(BUFSIZE);
    if (fgets(input, BUFSIZE, stdin)) {
        return strtok(input, "\n");
    }
    return NULL;
}*/

/**
 * function: login process from the client side
 * sd: socket descriptor
 **/
/*void authenticate(int sd) {
    char *input, desc[100];
    int code;

    // ask for user
    printf("username: ");
    input = read_input();

    // send the command to the server
    
    // relese memory
    free(input);

    // wait to receive password requirement and check for errors


    // ask for password
    printf("passwd: ");
    input = read_input();

    // send the command to the server


    // release memory
    free(input);

    // wait for answer and process it and check for errors

}*/

/**
 * function: operation get
 * sd: socket descriptor
 * file_name: file name to get from the server
 **/
/*void get(int sd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    FILE *file;

    // send the RETR command to the server

    // check for the response

    // parsing the file size from the answer received
    // "File %s size %ld bytes"
    sscanf(buffer, "File %*s size %d bytes", &f_size);

    // open the file to write
    file = fopen(file_name, "w");

    //receive the file



    // close the file
    fclose(file);

    // receive the OK from the server

}*/

/**
 * function: operation quit
 * sd: socket descriptor
 **/
/*void quit(int sd) {
     // Enviar comando QUIT del cliente
    send_msg(sd, "QUIT", NULL);
    // recibir respuesta del servidor
    if(!recv_msg(sd, 221, NULL))
        errx(10, "Incorrect logout");
    return;

}*/

/**
 * function: make all operations (get|quit)
 * sd: socket descriptor
 **/
/*void operate(int sd) {
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
}*/
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
        printf("Error en cantidad de argumentos");
    }
    if(!direccion_IP(argv[1])){
        printf("Error en ip");
    }
    if(!direccion_puerto(argv[2])){
        printf("Error en puerto");
    }

    // Creo un socket y chequeo errores
     if((sd = socket(AF_INET, SOCK_STREAM, 0))<0){
         printf("Error en la creacion del socket");
     }

    puerto = atoi(argv[2]); // Asignamos el puerto a la variable puerto
    // Obtenemos datos del servidor    
    addr.sin_family = AF_INET;
    addr.sin_port =  htons(puerto); 
    addr.sin_addr.s_addr = inet_addr(argv[1]);//Direccion

    // Conectamos y chequeamos errores
    if(connect(sd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        printf("Error, no se pudo conectar con el servidor.\n");
    }    
    // if receive hello proceed with authenticate and operate if not warning
    if(read(sd,buffer,sizeof(buffer))<0){
        printf("No se pudo leer el mensaje del servidor.\n");
    }
    // close socket
    close(sd);

    return 0;
}
