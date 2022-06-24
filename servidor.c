#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <err.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include<ctype.h>

#define BUFSIZE 512
#define CMDSIZE 4
#define PARSIZE 100
#define SRCPORT 20
#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_331 "331 Password required for %s\r\n"
#define MSG_230 "230 User %s logged in\r\n"
#define MSG_530 "530 Login incorrect\r\n"
#define MSG_221 "221 Goodbye\r\n"
#define MSG_550 "550 %s: no such file or directory\r\n"
#define MSG_299 "299 File %s size %ld bytes\r\n"
#define MSG_226 "226 Transfer complete\r\n"
#define MSG_200 "200 PORT command successful\r\n"
#define MSG_150 "150 Opening BINARY mode data connection for %s\r\n"
#define MSG_300 "300 Directorio creado con exito\r\n"
#define MSG_301 "301 No se pudo crear/eliminar el directorio. Corrobore si tiene elementos\r\n"
#define MSG_302 "302 No se pudo crear/eliminar el directorio, path existente\r\n"
#define MSG_303 "303 No se pudo crear/eliminar el directorio, demasiado largo\r\n"
#define MSG_304 "304 No se pudo crear/eliminar el directorio, el pathparent no tiene permitido escribirse\r\n"
#define MSG_305 "305 Directorio eliminado con exito\r\n"
#define MSG_306 "306 Cambio de directorio exitoso\r\n"
#define MSG_307 "307 Error: No se pudo cambiar de directorio\r\n"
#define MSG_308 "308 Directorio listado exitosamente en el servidor\r\n"


// Recibe los comandos del cliente. Ej: recv_cmd(sd, "USER", param), Retorna un booleano.
bool recv_cmd(int, char *, char *);
// Envia respuesta al cliente (...: argumentos variables para la economía de los formatos).
bool send_ans(int, char *, ...);

// Retorna el archivo solicitado.
void retr(int, struct sockaddr_in, char *);

// Chequea que las credenciales sean validas.
bool check_credentials(char *, char *);

// Autenticacion del usuario, si es valida devuelve true.
bool authenticate(int);

// Estructura de puerto.
struct sockaddr_in port(int, char *);

// Crear directorio
void mkdirec(int, char *);

// Borrar directorio
void rmdirec(int, char *);

// Cambiar de directorio
void cd(int, char *);

// Listar archivos de un directorio
void listar(int, char *);

// Ejecucion de comandos
void operate(int);

// Verifico si el puerto ingresado es valido (Digitos y el rango).
bool direccion_puerto(char *);


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
        }else{
        operate(sd);
        }
        // Cierro del lado del cliente
        close(sd);
        exit(0);
    }

    // cierro el server socket
    close(msd);

    return 0;
}

bool send_ans(int sd, char *message, ...){
    char buffer[BUFSIZE];

    va_list args;
    va_start(args, message);

    vsprintf(buffer, message, args);
    va_end(args);
    // Envio respuesta
    if(write(sd, buffer, strlen(buffer))<0){
        printf("Error de envio de respuesta");
        return false;
    }
    return true;
}

bool recv_cmd(int sd, char *operation, char *param) {
    char buffer[BUFSIZE], *token;
    int recv_s;
    
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
    if (token == NULL) {
        printf("Comando invalido");
        return false;
    } else {
        if(operation[0] == '\0'){
         strcpy(operation, token);
        }
        if(strcmp(operation, token)) {
            printf("Error al enviar el comando %s", operation);
            return false;
        }
        token = strtok(NULL, " ");
        if(token != NULL){
         strcpy(param, token);
        }
    }
    return true;
}

void mkdirec(int sd, char *file_path) {
    errno = 0;
      
    int ret = mkdir(file_path, S_IRWXU);
    
    if (ret == -1) {
        switch (errno) {
            case EACCES :
                send_ans(sd, MSG_304);
                return;
            case EEXIST:
                send_ans(sd, MSG_302);
                return;
            case ENAMETOOLONG:
                send_ans(sd, MSG_303);
                return;
            default:
                send_ans(sd, MSG_301);
                return;
        }
    }

    send_ans(sd, MSG_300);
    return;
}

void rmdirec(int sd, char *file_path) {
    errno = 0;
    
    int ret = rmdir(file_path);
    
    if (ret == -1) {
        switch (errno) {
            case EACCES :
                send_ans(sd, MSG_304);
                return;
            case EEXIST:
                send_ans(sd, MSG_302);
                return;
            case ENAMETOOLONG:
                send_ans(sd, MSG_303);
                return;
            default:
                send_ans(sd, MSG_301);
                return;
        }
    }

    send_ans(sd, MSG_305);
    return;
}

void cd(int sd, char *file_path){

	char s[100];
    
	chdir(file_path);
	
	if(chdir < 0){
	  // printing current working directory
      printf("Directorio actual:%s\n", getcwd(s, 100));
	  send_ans(sd, MSG_307);
	  return;
	}else{
	  // printing current working directory
      printf("Directorio actual:%s\n", getcwd(s, 100));
	  send_ans(sd, MSG_306);
      return;
	}
}

void listar(int sd, char *file_path){
	DIR *d;
	struct dirent *dir;
	
	d = opendir(file_path);
	
	if(d){
	  while((dir = readdir(d)) != NULL){
		printf("%s\n", dir->d_name);
	     }
		closedir(d);
          	send_ans(sd, MSG_308);
	  	return;
	 }else{
	  closedir(d);
	  return;
	}
}


void retr(int sd, struct sockaddr_in addr, char *file_path) {
    FILE *file;
    int bread;
    long fsize;
    char buffer[BUFSIZE];
    int srcsd;

    // chequeo si el archivo existe y si no envio un error
    file = fopen(file_path, "r");
    if (file == NULL){
        send_ans(sd, MSG_550, file_path);
        return;
    }

    // envio un mensaje con el largo del archivo
    fseek(file, 0L, SEEK_END);
    fsize = ftell(file);
    rewind(file);
    send_ans(sd, MSG_299, file_path, fsize);

    // Abro conexion con el cliente (creo socket)
    srcsd = socket(AF_INET, SOCK_STREAM, 0);
    if (srcsd < 0){
    	printf("No se pudo crear el socket");
    }
    
    if (connect(srcsd, (struct sockaddr *) &addr, sizeof(addr)) < 0){
    	printf("Error en la conexion");
    }
    
    // Envio el archivo
    while(!feof(file)) {
        bread = fread(buffer, 1, BUFSIZE, file);
        if (write(srcsd, buffer, bread) < 0){
           printf("Error de envio de datos");
        }
    }

    // cierro conexion con el cliente
    close(srcsd);

    // Envio el mensaje que se completo la transferencia
    send_ans(sd, MSG_226);
    
    return;
}

bool authenticate(int sd) {
    char user[PARSIZE], pass[PARSIZE];

    // Espero a recibir el usuario
    if(!recv_cmd(sd, "USER", user)){
        printf("Error en la espera de la recepcion del usuario\n");
        return false;
    }

    // Pregunto por la contrasenia
    if(!send_ans(sd, MSG_331, user)){
        printf("Error al preguntar por la contrasenia");
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


struct sockaddr_in port(int sd, char *socketdata){
    struct sockaddr_in addr;
    int puerto, i, j, count;
    char *ip, *aux1, *aux2;
    ip = (char*)malloc(30*sizeof(char));
    aux1 = (char*)malloc(4*sizeof(char));
    aux2 = (char*)malloc(4*sizeof(char));

    i = j = 0;
    count=0;

    while(true){
        if (*(socketdata+i) == ',') count++;

        if (count<=3){
            *(ip+j) = *(socketdata+i);
            if (*(ip+j) == ',') *(ip+j) = '.';
            j++;
        }
        if (*(socketdata+i) == ',' && count==4){
            *(ip+j) = '\0';
            j=0;
            i++;
            continue;
        }

        if(count==4){
            *(aux1+j) = *(socketdata+i);
            j++;
        }

        if (*(socketdata+i) == ',' && count==5){
            *(aux1+j) = '\0';
            j=0;
            i++;
            continue;
        }

        if(count==5){
            *(aux2+j) = *(socketdata+i);
            j++;
        }

        if (*(socketdata+i) == '\0'){
            *(aux2+j) = '\0';
            break;
        }
        i++;
    }
    puerto = 256 * atoi(aux1) + atoi(aux2);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(puerto);

    free(ip);
    free(aux1);
    free(aux2);

    send_ans(sd, MSG_200);

    return addr;
}

bool check_credentials(char *user, char *pass) {
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

     // Busco las cadenas de las credenciales
    line = (char *) malloc(100*sizeof(char));//reservo memoria dinamica
    len = 100;
    
    // Leo el archivo linea a linea
    while(getline(&line, &len, file)>0){
        if(strcmp(cred, line) == 0){
            found = true;
            break;
        }
    }

    // Cierro el archivo
    fclose(file);

    // Retorno el estado de la busqueda
    return found;
}

void operate(int sd){
    char op[PARSIZE+1], param[PARSIZE];
    struct sockaddr_in client_address;

    while (true) {
        op[0] = param[0] = '\0';
        
        //Verifico los comandos del cliente
        if(!recv_cmd(sd, op, param)){
            printf("Error al recibir el comando");
            break;
        }
        
        if (strcmp(op, "PORT") == 0 ){
            client_address = port(sd, param);
        }else if (strcmp(op, "RETR") == 0) {        
            retr(sd, client_address, param);
        }else if (strcmp(op, "MKDIR") == 0){       	
            mkdirec(sd, param);
        }else if (strcmp(op, "RMDIREC" ) == 0){
            rmdirec(sd, param);
        }else if (strcmp(op, "CD" ) == 0){
            cd(sd, param);
        }else if (strcmp(op, "LISTAR" ) == 0){
            listar(sd, param);
        }else if (strcmp(op, "QUIT") == 0) {
            // Saludo y cierro la conexion
            send_ans(sd, MSG_221);
            close(sd);
            printf("User logged out.\n");
            break;
        }else{   // invalid command
            printf("Comando: %s invalido\n", op);
            
        }
    }
 
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
