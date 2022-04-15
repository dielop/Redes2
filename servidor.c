#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <err.h>
#include <ctype.h>

#include <netinet/in.h>

#define BUFSIZE 512
#define CMDSIZE 4
#define PARSIZE 100

#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_331 "331 Password required for %s\r\n"
#define MSG_230 "230 User %s logged in\r\n"
#define MSG_530 "530 Login incorrect\r\n"
#define MSG_221 "221 Goodbye\r\n"
#define MSG_550 "550 %s: no such file or directory\r\n"
#define MSG_299 "299 File %s size %ld bytes\r\n"
#define MSG_226 "226 Transfer complete\r\n"

/**
 * function: receive the commands from the client
 * sd: socket descriptor
 * operation: \0 if you want to know the operation received
 *            OP if you want to check an especific operation
 *            ex: recv_cmd(sd, "USER", param)
 * param: parameters for the operation involve
 * return: only usefull if you want to check an operation
 *         ex: for login you need the seq USER PASS
 *             you can check if you receive first USER
 *             and then check if you receive PASS
 **/
bool recv_cmd(int sd, char *operation, char *param) {
    char buffer[BUFSIZE], *token;
    int recv_s;

    // Recibo los comandos en el buffer y chequeo errores
    recv_s = read(sd,buffer,BUFSIZE);
    if(recv_s < 0 || recv==0){
        printf("Error al leer el buffer o buffer vacio\n");
        return false;
    }
    
    // Elimino los caracteres de terminacion del buffer
    buffer[strcspn(buffer, "\r\n")] = 0;

    // complex parsing of the buffer
    // extract command receive in operation if not set \0
    // extract parameters of the operation in param if it needed
    token = strtok(buffer, " ");
    if (token == NULL || strlen(token) < 4) {
        warn("not valid ftp command");
        return false;
    } else {
        if (operation[0] == '\0') strcpy(operation, token);
        if (strcmp(operation, token)) {
            warn("abnormal client flow: did not send %s command", operation);
            return false;
        }
        token = strtok(NULL, " ");
        if (token != NULL) strcpy(param, token);
    }
    return true;
}

/**
 * function: send answer to the client
 * sd: file descriptor
 * message: formatting string in printf format
 * ...: variable arguments for economics of formats
 * return: true if not problem arise or else
 * notes: the MSG_x have preformated for these use
 **/
bool send_ans(int sd, char *message, ...){
    char buffer[BUFSIZE];

    va_list args;
    va_start(args, message);

    vsprintf(buffer, message, args);
    va_end(args);
    // send answer preformated and check errors
    if(write(sd, buffer, strlen(buffer))<0){
        printf("Error de envio de respuesta");
        return false;
    }
    return true;
}

/**
 * function: RETR operation
 * sd: socket descriptor
 * file_path: name of the RETR file
 **/

/*void retr(int sd, char *file_path) {
    FILE *file;    
    int bread;
    long fsize;
    char buffer[BUFSIZE];

    // check if file exists if not inform error to client

    // send a success message with the file length

    // important delay for avoid problems with buffer size
    sleep(1);

    // send the file

    // close the file

    // send a completed transfer message
}*/
/**
 * funcion: check valid credentials in ftpusers file
 * user: login user name
 * pass: user password
 * return: true if found or false if not
 **/
bool check_credentials(char *user, char *pass) {
    // Variables
    FILE *file;
    char *path = "./ftpusers", *line = NULL, cred[100];
    size_t len = 0;
    bool found = false;

    // Hago la cadena de credenciales
    strcpy(cred, user);
    strcat(cred, ":");
    strcat(cred, pass);
    strcat(cred, "\n");
    
    // Chequeo si el archivo ftpusers existe
    file = fopen(path, "r");
   /* if(file = NULL){
        printf("Error al abrir el archivo de usuarios\n");
    }*/
    
    // Busco las cadenas de las credenciales 
    line = (char *) malloc(100*sizeof(char));//reservo memoria dinamica
    len = 100;
    
    // Leo el archivo linea a linea
    while(getline(&line, &len, file)>0){
   // printf("Entre aca porque era mayor a 0\n");
        if(strcmp(cred, line) == 0){
           found = true;
           //printf("Se encontro el usuario\n");
           break;
        }
    }
    // Cierro el archivo
    fclose(file);

    // Retorno el estado de la busqueda
    printf("%d", found);
    return found;
}

/**
 * function: login process management
 * sd: socket descriptor
 * return: true if login is succesfully, false if not
 **/
bool authenticate(int sd) {
    char user[PARSIZE], pass[PARSIZE];

    // Espero a recibir el usuario
    if(!recv_cmd(sd, "USER", user)){
        printf("Error en la espera de la recepcion del usuario\n");
        return false;
    }
    // Pregunto por la contrasenia
    if(!send_ans(sd, MSG_331, user)){
        printf("Error al preguntar por la contrasenia\n");
        return false;
    }
    // Espero para recibir la contrasenia
    if(!recv_cmd(sd, "PASS", pass)){
        printf("Error al recibir la contrasenia");
        return false;
    }
    // confirmo el login
    if(check_credentials(user, pass)){
        if(!send_ans(sd, MSG_230, user)){
            printf("Error al enviar la confirmacion del ingreso");
            return false;
        }
        printf("User: %s conectado.\n", user);
        return true;
    }
    // Si las credenciales no fueron correctamente chequeadas, devuelvo el acceso denegado
    if(!send_ans(sd, MSG_530)){
        printf("Error al enviar el acceso denegado\n");
        return false;
    }
}

/**
 *  function: execute all commands (RETR|QUIT)
 *  sd: socket descriptor
 **/

/*void operate(int sd) {
    char op[CMDSIZE], param[PARSIZE];

    while (true) {
        op[0] = param[0] = '\0';
        // check for commands send by the client if not inform and exit


        if (strcmp(op, "RETR") == 0) {
            retr(sd, param);
        } else if (strcmp(op, "QUIT") == 0) {
            // send goodbye and close connection




            break;
        } else {
            // invalid command
            // furute use
        }
    }
}*/

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
 *         ./mysrv <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {

    // Espacio de reserva de sockets y variables
    char buffer[BUFSIZE];
    int msd,sd; //socket cliente y socket server, puerto
    struct sockaddr_in masterAddr, servAddr;
    socklen_t s_length;
    int puerto;
    bool autenti;
    
    // Chequea argumentos de entrada
    if(argc!=2){
        printf("Error en el numero de argumentos");
    }
    // Chequea el puerto
    if(!direccion_puerto(argv[1])){
        printf("Puerto invalido");
    }
    // Creo el socket del servidor y si hay errores
    msd = socket(AF_INET, SOCK_STREAM, 0);
    if(msd<0){
        printf("Error al crear el socket.\n");
    }
    
    puerto = atoi(argv[1]); // Recupero el puerto en la variable puerto
    
    // Asigno puerto e ip al socket y chequeo errores
    masterAddr.sin_family = AF_INET;
    masterAddr.sin_port = htons(puerto); //Puerto que usamos ingresado como arg recuperado mas arriba
    masterAddr.sin_addr.s_addr = INADDR_ANY;
    
    if(bind(msd, (struct sockaddr*)&masterAddr, sizeof(masterAddr)) < 0){
        printf("Error en bind de conexion.\n");
    }
    
    // Hace un listen
    if(listen(msd, 10)<0){
        printf("Error en listen");
    }
    
    // bucle
    while (true) {
        // Aceptamos conexiones secuencialmente y checkeamos los errores
        s_length = sizeof(servAddr);
        if((sd = accept(msd, (struct sockaddr*)&servAddr, &s_length)) < 0){
                printf("Error de conexion en la aceptacion");
        }
        // envio mensaje de primera conexion
        send_ans(sd, MSG_220);

        // Operacion de autenticacion
        autenti = authenticate(sd);        
        if(!autenti){
            printf("Error de autenticacion\n");
        }
        // Cierro del lado del cliente
        close(sd);
    }

    // cierro el server socket
    close(msd);

    return 0;
}
