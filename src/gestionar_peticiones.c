#include "gestionar_peticiones.h"
#include "lines.h"
#include <netinet/in.h>
#include <fcntl.h>        
#include <sys/stat.h>
#include <sys/file.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "proxy-rpc.h"

extern pthread_mutex_t mi_mutex;

unsigned int id_mensaje_actual = 0;

int registrar_peticion(char *nombre_usuario ,char *op, char *fichero){
    
    // Cogemos la variable de entorno definida y mandamos la peticion al servidor
    char *host  = getenv("LOG_RPC_IP");
    int ret;
    int resultado = login_peticion(host, nombre_usuario, op, fichero);
    
    return resultado;
}

char** leer_users(int* num_users_conn){
    DIR *directorio;
    struct dirent *entrada;
    char* ruta_carpeta = "clientes";
    int current_size = BUFFER_SIZE;
    char ** users = malloc(sizeof(char*) * current_size);
    if (users == NULL){
        return NULL;
    }

    directorio = opendir(ruta_carpeta);
    if (directorio == NULL) {
        printf("Error: No se pudo abrir la carpeta '%s'.\n", ruta_carpeta);
        free(users);
        return NULL;
    }

    while ((entrada = readdir(directorio)) != NULL) {

        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue; 
        }

        if (entrada->d_type == DT_REG) {
            char ruta[BUFFER_SIZE + 10];
            sprintf(ruta,"clientes/%s", entrada->d_name);
            int fd = open(ruta, O_RDONLY, 0644);

            if (fd < 0) {
                for (int i = 0; i < *num_users_conn; i++){
                    free(users[i]);
                }   
                free(users);
                closedir(directorio);
                return NULL;
            }

            if(flock(fd, LOCK_SH) == -1){
                // No se ha podido bloquear el archivo para leer la información, devolvemos error
                for (int i = 0; i < *num_users_conn; i++){
                    free(users[i]);
                } 
                free(users);
                close(fd);
                closedir(directorio);
                return NULL;
            }
            struct info_usuario datos_usuario;
            if(readFull(fd, (char*) &datos_usuario, sizeof(struct info_usuario)) != 0){
                for (int i = 0; i < *num_users_conn; i++){
                    free(users[i]);
                } 
                free(users);
                flock(fd, LOCK_UN);
                close(fd);
                closedir(directorio);
                return NULL;
            }
            if (datos_usuario.estado == 1){
                if (*num_users_conn >= current_size){
                    // Hemos llegado al límite de usuarios que podemos enviar, devolvemos error
                    char **tmp = realloc(users, sizeof(char*) * (current_size + 10));
                    current_size += 10;
                    if (tmp == NULL) {
                        for (int i = 0; i < *num_users_conn; i++){ 
                            free(users[i]);
                        }
                        free(users);
                        closedir(directorio);
                        close(fd);
                        return NULL;
                    }
                    users = tmp;
                }
                users[*num_users_conn] = strdup(datos_usuario.nombre_cliente);
                (*num_users_conn)++;
            }
            flock(fd, LOCK_UN);
            close(fd);
            }
        }

    closedir(directorio);
    return users;
}

void gestionar_register(struct peticion datos_recibidos){

    // A continuación el servidor debe obtener el nombre de usuario a registrar
    char nombre_usuario[BUFFER_SIZE];

    // El codigo de error es un unico byte
    char codigo;
    int sd = datos_recibidos.socket_cliente;
    // Obtenemos el usuario
    if(readLine(sd,nombre_usuario,BUFFER_SIZE) > 0){
        // Hemos obtenido texto, asumimos que es el usuario a registrar
        
        if(mkdir("clientes",0700) == -1){
            
            if(errno != EEXIST){
                // Hubo un error que no fue debido a que el directorio ya existía
                
                // Enviamos el mensaje de error y finalizamos la ejecución
                codigo = 2;
                // Enviamos el codigo a destino
                sendMessage(sd,&codigo,1);
                // Mostramos el mensaje de error
                printf("s> REGISTER %s FAIL\n", nombre_usuario);
                return;
            }
            // Si errno == EEXIST podemos intentar crear el usuario
        }
        
        // Verificamos que no exista ningún usuario con ese nombre. Para ello accedemos al directorio
        // y comprobamos si el archivo ya existe
        
        // Formamos la ruta del archivo para ese usuario
        char ruta[BUFFER_SIZE + 10];
        sprintf(ruta,"clientes/%s",nombre_usuario);

        int fd = open(ruta, O_CREAT | O_EXCL | O_WRONLY, 0644);

        if(fd < 0){
            // El archivo ya existe
            
            // Enviamos el codigo de error y finalizamos la ejecución
            codigo = 1;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error
            printf("s> REGISTER %s FAIL\n", nombre_usuario);
            return;
        }

        // Se ha podido crear el archivo por lo que ahora escribimos la información del usuario
        struct info_usuario datos_usuario;
        memset(&datos_usuario,0,sizeof(datos_usuario));
        // Copiamos el nombre del usuario
        strncpy(datos_usuario.nombre_cliente,nombre_usuario,255);
        // Establecemos que esté desconectado por defecto
        datos_usuario.estado = 0;
        datos_usuario.ultimo_id = 0;

        // Inicialmente la IP está vacía
        strcpy(datos_usuario.ip,"");
        // De momento el cliente no tiene puerto asociado
        datos_usuario.puerto_escucha_cliente = 0;

        // Ahora guardamos la información en el fichero

        if(flock(fd,LOCK_EX) == -1){
            // NO se ha podido bloquear el archivo para escribir la información.
            codigo = 2;
            sendMessage(sd,&codigo,1);
            printf("s> REGISTER %s FAIL\n", nombre_usuario);
            close(fd);
            return;
        }
        
        if(writeFull(fd,(char* ) &datos_usuario,sizeof(datos_usuario)) != 0){
            codigo = 2;
            sendMessage(sd,&codigo,1);
            printf("s> REGISTER %s FAIL\n", nombre_usuario);
            flock(fd,LOCK_UN);
            close(fd);
            return;
        }
        // Ahora no hay mensajes pendientes por lo que no tenemos que escribir nada más
        flock(fd,LOCK_UN); 
        close(fd);
        // En este caso, se creo el usuario correctamente por lo que enviamos el código y printeamos
        codigo = 0;
        sendMessage(sd,&codigo,1);
        printf("s> REGISTER %s OK\n",nombre_usuario);

        // Consideramos que solamente se guarda la instrucción en el log si ocurre de forma correcta
        registrar_peticion(nombre_usuario,"REGISTER","");

    }else{
        // Enviamos el codigo de error y finalizamos la ejecución
            codigo = 2;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> REGISTER unknown_user FAIL\n");
            
    }

}


void gestionar_unregister(struct peticion datos_recibidos){

    // Primero se recibe el nombre de usuario que se desea borrar
    char nombre_usuario[BUFFER_SIZE];

    // El codigo de error es un unico byte
    char codigo;
    int sd = datos_recibidos.socket_cliente;
    // Obtenemos el usuario
    if(readLine(sd,nombre_usuario,BUFFER_SIZE) > 0){
        
        // Verificamos que exista un usuario con ese nombre. Para ello accedemos al directorio
        // y comprobamos si el archivo ya existe
        
        // Formamos la ruta del archivo para ese usuario
        char ruta[BUFFER_SIZE + 10];
        sprintf(ruta,"clientes/%s",nombre_usuario);

        int fd = open(ruta,O_RDONLY);

        if(fd < 0){
            // El usuario no existe, se envia código 1 al cliente
            codigo = 1;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error
            printf("s> UNREGISTER %s FAIL\n", nombre_usuario);
            return;
        }

        // Añadimos LOCK_NB para que si se está usando otro lock por otro proceso, entonces no espere
        // y que devuelva -1
        if(flock(fd,LOCK_EX | LOCK_NB) == -1){
            // No se pudo bloquear el file porque ya está siendo usado, devolvemos error
            codigo = 2;
            sendMessage(sd, &codigo,1);
            printf("s> UNREGISTER %s FAIL\n", nombre_usuario);
            close(fd);
            return;

        }

        // Aquí ya tenemos el archivo bloqueado por lo que lo podemos borrar

        if(remove(ruta) == 0){
            // Se borró de forma exitosa
            codigo = 0;
            sendMessage(sd,&codigo,1);
            printf("s> UNREGISTER %s OK\n", nombre_usuario);

            // Consideramos que solamente se guarda la instrucción en el log si ocurre de forma correcta
            registrar_peticion(nombre_usuario,"UNREGISTER","");
            

        } else{
             codigo = 2;
            sendMessage(sd, &codigo,1);
            printf("s> UNREGISTER %s FAIL\n", nombre_usuario);
        }
        flock(fd,LOCK_UN);
        close(fd);

    }else{
        // Enviamos el codigo de error y finalizamos la ejecución
            codigo = 2;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> UNREGISTER unknown_user FAIL\n");
            
    }

}

void gestionar_users(struct peticion datos_recibidos){
     // El codigo de error es un unico byte
    char codigo;
    int sd = datos_recibidos.socket_cliente;
    char nombre_usuario[BUFFER_SIZE];
    char num_users_str[4];
    if (readLine(sd,nombre_usuario,BUFFER_SIZE) > 0){    
        // Verificamos que exista un usuario con ese nombre. Para ello accedemos al directorio y comprobamos si el archivo ya existe
        char ruta[BUFFER_SIZE + 10];
        sprintf(ruta,"clientes/%s",nombre_usuario);

        int fd = open(ruta,O_RDWR);

        if(fd < 0){
            // El usuario no existe, se envia código 1 al cliente
            codigo = 1;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error
            printf("s> CONNECTEDUSERS FAIL\n");
            return;
        }

         if(flock(fd,LOCK_SH) == -1){
            // Ha ocurrido algún error inesperado
            codigo = 3;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> CONNECTEDUSERS FAIL\n");
            close(fd);
            return;
        }

        // Ahora leemos el archivo para obtener los datos
        struct info_usuario datos_usuario;

        if(readFull(fd, (char*) &datos_usuario, sizeof(struct info_usuario)) != 0){
            // No se ha podido obtener la info
            codigo = 3;
            sendMessage(sd, &codigo, 1);
            printf("s> CONNECTEDUSERS FAIL\n");
            flock(fd, LOCK_UN);
            close(fd);
            return;

        }

        if(datos_usuario.estado == 0){
            // El usuario no estaba conectado
            codigo = 2;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> CONNECTEDUSERS FAIL\n");
            flock(fd, LOCK_UN);
            close(fd);
            return;
        }

        if(strcmp(datos_recibidos.ip, datos_usuario.ip) != 0){
            codigo = 2;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> CONNECTEDUSERS FAIL\n");
            flock(fd, LOCK_UN);
            close(fd);
            return;
        }
        flock(fd, LOCK_UN);
        close(fd);
        int num_users_conn = 0;
        char ** users = leer_users(&num_users_conn);
        if (users == NULL){
            codigo = 2;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer los usuarios conectados
            printf("s> CONNECTEDUSERS FAIL\n");
            return;
        }else{
            codigo = 0;
            sendMessage(sd,&codigo,1);
            printf("s> CONNECTEDUSERS OK\n");
            // Finalizamos la ejecución, se hicieron todos los cambios
            sprintf(num_users_str, "%03d", num_users_conn);
            sendMessage(sd, num_users_str, 4);
            for (int i = 0; i < num_users_conn; i++){
                sendMessage(sd, users[i], strlen(users[i]) + 1);
                free (users[i]);
            }
            free(users);

            // Consideramos que solamente se guarda la instrucción en el log si ocurre de forma correcta
            registrar_peticion(nombre_usuario,"USERS","");
            return;
        }
}
}

void gestionar_connect(struct peticion datos_recibidos){

    // El codigo de error es un unico byte
    char codigo;
    int sd = datos_recibidos.socket_cliente;
    char buffer_recepcion[BUFFER_SIZE];
    // Obtenemos el usuario
    if(readLine(sd,buffer_recepcion,BUFFER_SIZE) > 0){

          // Antes de comprobar al usuario, también recibimos el puerto
        char nombre_usuario[BUFFER_SIZE];

        // Guardamos el nombre de usuario
        strcpy(nombre_usuario,buffer_recepcion);

        if(readLine(sd,buffer_recepcion,BUFFER_SIZE) <= 0){
            // No se ha conseguido obtener el puerto
            codigo = 3;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> CONNECT %s FAIL\n",nombre_usuario);
            return;
        }

        // Nos guardamos el puerto
        int puerto = atoi(buffer_recepcion);
        if(puerto == 0 || puerto > 65535){
            // El puerto no es valido
            codigo = 3;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> CONNECT %s FAIL\n",nombre_usuario);
            return;
            
        }

        // Verificamos que exista un usuario con ese nombre. Para ello accedemos al directorio y comprobamos si el archivo ya existe
        char ruta[BUFFER_SIZE + 10];
        sprintf(ruta,"clientes/%s",nombre_usuario);

        int fd = open(ruta,O_RDWR);

        if(fd < 0){
            // El usuario no existe, se envia código 1 al cliente
            codigo = 1;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error
            printf("s> CONNECT %s FAIL\n", nombre_usuario);
            return;
        }

        // Ahora vamos a bloquear el archivo para asegurarnos que nadie lo accede mientras lo modificamos
        if(flock(fd,LOCK_EX) == -1){
            // Ha ocurrido algún error inesperado
            codigo = 3;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> CONNECT %s FAIL\n",nombre_usuario);
            close(fd);
            return;
        }
        
        // Aquí ya hemos bloqueado el archivo por lo que leemos la info
        struct info_usuario datos_usuario;
        if(readFull(fd, (char*) &datos_usuario, sizeof(struct info_usuario)) != 0){
            // No se ha podido obtener la info
            codigo = 3;
            sendMessage(sd, &codigo, 1);
            printf("s> CONNECT %s FAIL\n",nombre_usuario);
            flock(fd, LOCK_UN);
            close(fd);
            return;

        }

        if(datos_usuario.estado == 1){
            // El usuario estaba conectado
            codigo = 2;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> CONNECT %s FAIL\n",nombre_usuario);
            flock(fd, LOCK_UN);
            close(fd);
            return;
        }
       
        // Aquí significa que el usuario estaba desconectado por lo que actualizamos sus datos
        datos_usuario.puerto_escucha_cliente = puerto;
        datos_usuario.estado = 1;
        strcpy(datos_usuario.ip,datos_recibidos.ip);


        // Ahora intentamos enviar los mensajes en caso de que hayan
        int num_fallidos = 0;
        struct mensaje mensajes_fallidos[100]; // Array fijo grande para guardar mensajes fallidos
        struct mensaje mensaje_obtenido;

        while(readFull(fd,&mensaje_obtenido,sizeof(struct mensaje)) == 0){
            // Hemos obtenido un mensaje, lo preparar para enviar
            if (gestionar_envio_mensajes(datos_usuario, mensaje_obtenido) == -1) {
                // No se ha podido enviar el mensaje al destinatario, lo dejamos para la próxima conexión
                mensajes_fallidos[num_fallidos] = mensaje_obtenido;
                num_fallidos++;
            }
        }
        
        // Nos movemos al inicio del archivo y escribimos la infomación actualizada
        lseek(fd,0,SEEK_SET);
        if(writeFull(fd,(char*)&datos_usuario,sizeof(struct info_usuario)) != 0){

            // No se pudo escribir en el archivo
            codigo = 3;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> CONNECT %s FAIL\n",nombre_usuario);
            flock(fd, LOCK_UN);
            close(fd);
            return;

        }

        // Enviamos al usuario el éxito de la conexión cuando ya se ha actualizado su info
        codigo = 0;
        sendMessage(sd,&codigo,1);
        printf("s> CONNECT %s OK\n",nombre_usuario);

        // Consideramos que solamente se guarda la instrucción en el log si ocurre de forma correcta
        registrar_peticion(nombre_usuario,"CONNECT","");

        for(int i = 0; i < num_fallidos; i++){
            if(writeFull(fd,&mensajes_fallidos[i],sizeof(struct mensaje)) != 0){
                // Hubo un error en reescribir dicho mensaje
                codigo = 3;
                sendMessage(sd, &codigo, 1);
                // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
                printf("s> CONNECT %s FAIL\n",nombre_usuario);
                flock(fd, LOCK_UN);
                close(fd);
                return;
                }
        }

        size_t nuevo_size_archivo = sizeof(struct info_usuario) + (sizeof(struct mensaje) * num_fallidos);
        // Establecemos el nuevo size para eliminar el contenido que pueda ir despues
        ftruncate(fd, nuevo_size_archivo);
        // Finalizamos la ejecución, se hicieron todos los cambios
        
        flock(fd,LOCK_UN);
        close(fd);

    }else{
        // No se ha podido obtener el nombre de usuario
            codigo = 3;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> CONNECT unknown_user FAIL\n");
            
    }
}



void gestionar_disconnect(struct peticion datos_recibidos){
     // El codigo de error es un unico byte
    char codigo;
    int sd = datos_recibidos.socket_cliente;
    char nombre_usuario[BUFFER_SIZE];
    // Obtenemos el usuario
    if(readLine(sd,nombre_usuario,BUFFER_SIZE) > 0){  

        // Verificamos que exista un usuario con ese nombre. Para ello accedemos al directorio y comprobamos si el archivo ya existe
        char ruta[BUFFER_SIZE + 10];
        sprintf(ruta,"clientes/%s",nombre_usuario);

        int fd = open(ruta,O_RDWR);

        if(fd < 0){
            // El usuario no existe, se envia código 1 al cliente
            codigo = 1;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error
            printf("s> DISCONNECT %s FAIL\n", nombre_usuario);
            return;
        }
        // Si el archivo existe, tenemos que cambiar sus datos para hacer que esté desconectado
         if(flock(fd,LOCK_EX) == -1){
            // Ha ocurrido algún error inesperado
            codigo = 3;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> DISCONNECT %s FAIL\n", nombre_usuario);
            close(fd);
            return;
        }

        // Ahora leemos el archivo para obtener los datos
        struct info_usuario datos_usuario;

         if(readFull(fd, (char*) &datos_usuario, sizeof(struct info_usuario)) != 0){
            // No se ha podido obtener la info
            codigo = 3;
            sendMessage(sd, &codigo, 1);
            printf("s> DISCONNECT %s FAIL\n",nombre_usuario);
            flock(fd, LOCK_UN);
            close(fd);
            return;

        }

        if(datos_usuario.estado == 0){
            // El usuario no estaba conectado
            codigo = 2;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> DISCONNECT %s FAIL\n",nombre_usuario);
            flock(fd, LOCK_UN);
            close(fd);
            return;
        }

        if(strcmp(datos_recibidos.ip, datos_usuario.ip) != 0){
            codigo = 3;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> DISCONNECT %s FAIL\n",nombre_usuario);
            flock(fd, LOCK_UN);
            close(fd);
            return;
        }
        strcpy(datos_usuario.ip,"");
        // Ponemos el estado a desconectado
        datos_usuario.estado = 0;
        datos_usuario.puerto_escucha_cliente = -1;

        lseek(fd,0,SEEK_SET);
        if(writeFull(fd,(char*)&datos_usuario,sizeof(struct info_usuario)) != 0){
            // No se pudo escribir en el archivo
            codigo = 3;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            printf("s> DISCONNECT %s FAIL\n",nombre_usuario);
            flock(fd, LOCK_UN);
            close(fd);
            return;

        }

         // Enviamos al usuario el éxito de la conexión cuando ya se ha actualizado su info
        codigo = 0;
        sendMessage(sd,&codigo,1);
        printf("s> DISCONNECT %s OK\n",nombre_usuario);

        // Consideramos que solamente se guarda la instrucción en el log si ocurre de forma correcta
        registrar_peticion(nombre_usuario,"DISCONNECT","");

        // Finalizamos la ejecución, se hicieron todos los cambios
        flock(fd,LOCK_UN);
        close(fd); 

    }else{
        // No se ha conseguido recibir el nombre de usuario
        codigo = 3;
        sendMessage(sd, &codigo, 1);
        // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
        printf("s> DISCONNECT unknown_user FAIL\n");
    }

}

// Esta función se encargará de preparar el mensaje a enviar y enviarlo
void gestionar_mensajes(struct peticion datos_recibidos){
         // El codigo de error es un unico byte
    char codigo;
    int sd = datos_recibidos.socket_cliente;
    char nombre_usuario_remitente[BUFFER_SIZE];
    char nombre_usuario_destino[BUFFER_SIZE];
    char text_message[256];
    // Obtenemos el usuario
    if(readLine(sd,nombre_usuario_remitente,BUFFER_SIZE) > 0){  
        if (readLine(sd,nombre_usuario_destino,BUFFER_SIZE) <= 0){
            // No se ha conseguido obtener el nombre del destinatario
            codigo = 2;
            sendMessage(sd, &codigo, 1);
            return;
        }

        if (readLine(sd,text_message,256) <= 0){
            // No se ha conseguido obtener el mensaje
            codigo = 2;
            sendMessage(sd, &codigo, 1);
            return;
        }

        // Verificamos que exista un usuario con ese nombre. Para ello accedemos al directorio y comprobamos si el archivo ya existe
        char ruta_destino[BUFFER_SIZE + 10];
        sprintf(ruta_destino,"clientes/%s", nombre_usuario_destino);

        int fd = open(ruta_destino, O_RDWR);

        if(fd < 0){
            // El usuario remitente no existe, se envia código 1 al cliente
            codigo = 1;
            sendMessage(sd, &codigo, 1);
            return;
        }

         if(flock(fd,LOCK_EX) == -1){
            // Ha ocurrido algún error inesperado
            codigo = 2;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
            close(fd);
            return;
        }

        // Ahora leemos el archivo para obtener los datos
        struct info_usuario datos_usuario;

        if(readFull(fd, (char*) &datos_usuario, sizeof(struct info_usuario)) != 0){
            // No se ha podido obtener la info
            codigo = 2;
            sendMessage(sd, &codigo, 1);
            flock(fd, LOCK_UN);
            close(fd);
            return;

        }

        struct mensaje mensaje_a_enviar;
        pthread_mutex_lock(&mi_mutex);
        mensaje_a_enviar.id = id_mensaje_actual++ % 999;
        pthread_mutex_unlock(&mi_mutex);
        strncpy(mensaje_a_enviar.usuario_origen, nombre_usuario_remitente, 255);
        strncpy(mensaje_a_enviar.texto_mensaje, text_message, 255);
        char str_id[4];
        sprintf(str_id, "%03d", mensaje_a_enviar.id);
        if(datos_usuario.estado == 0){
            // El usuario no esta conectado, se guarda en el archivo para que se envie cuando se conecte

            lseek(fd,0,SEEK_END);
            if(writeFull(fd,(char*) &mensaje_a_enviar, sizeof(struct mensaje)) != 0){
                // No se ha podido escribir el mensaje
                codigo = 2;
                sendMessage(sd, &codigo, 1);
                flock(fd, LOCK_UN);
                close(fd);
                return;
            }
            codigo = 0;
            sendMessage(sd,&codigo,1);
            sendMessage(sd, str_id, 4);
            flock(fd,LOCK_UN);
            close(fd); 
            printf("s> MESSAGE %s FROM %s TO %s STORED\n",str_id, nombre_usuario_remitente, nombre_usuario_destino);
            
            // Consideramos que solamente se guarda la instrucción en el log si ocurre de forma correcta
            registrar_peticion(nombre_usuario_remitente,"SEND","");

            return;
        }
        if (gestionar_envio_mensajes(datos_usuario, mensaje_a_enviar) == -1) {
            // No se ha podido enviar el mensaje al destinatario
            codigo = 2;
            printf("s> Error no se pudo enviar mensaje\n");
            sendMessage(sd, &codigo, 1);
            flock(fd, LOCK_UN);
            close(fd);
            return;
        }
        codigo = 0;
        sendMessage(sd,&codigo,1);
        sendMessage(sd, str_id, 4);
        printf("s> SEND MESSAGE %s FROM %s TO %s\n", str_id, nombre_usuario_remitente, nombre_usuario_destino);
        
        // Consideramos que solamente se guarda la instrucción en el log si ocurre de forma correcta
        registrar_peticion(nombre_usuario_remitente,"SEND","");

        // Ahora obtenemos la información del remitente para enviarle el ACK
        char ruta_remitente[BUFFER_SIZE + 10];
        sprintf(ruta_remitente,"clientes/%s", nombre_usuario_remitente);
        int fd_remitente = open(ruta_remitente, O_RDONLY);
        
        if(fd_remitente >= 0) {
            if(flock(fd_remitente, LOCK_SH) == 0) {
                struct info_usuario datos_remitente;
                if(readFull(fd_remitente, (char*) &datos_remitente, sizeof(struct info_usuario)) == 0) {
                    // Intentamos enviar el ACK al remitente
                    int sd_ack = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                    if(sd_ack >= 0) {
                        struct sockaddr_in direccion_remitente;
                        direccion_remitente.sin_family = AF_INET;
                        direccion_remitente.sin_port = htons(datos_remitente.puerto_escucha_cliente);
                        inet_pton(AF_INET, datos_remitente.ip, &direccion_remitente.sin_addr);
                        
                        if(connect(sd_ack, (struct sockaddr*) &direccion_remitente, sizeof(direccion_remitente)) == 0) {
                            char message[BUFFER_SIZE] = "SEND_MESS_ACK";
                            sendMessage(sd_ack, message, 14);
                            char str_id[4];
                            sprintf(str_id, "%03d", mensaje_a_enviar.id);
                            sendMessage(sd_ack, str_id, 4);
                        }
                        close(sd_ack);
                    }
                }
                flock(fd_remitente, LOCK_UN);
            }
            close(fd_remitente);
        }
        
        flock(fd,LOCK_UN);
        close(fd); 

    }else{
        // No se ha conseguido recibir el nombre de usuario
        codigo = 2;
        sendMessage(sd, &codigo, 1);
    }

    // nombre del cliente está en el struct datos_usuario
    // el segundo parámetro es el mensaje a enviar, se devuelve -1 si no se consigue enviar el mensaje
    // ESTA FUNCION SE ENCARGARA DE MOSTRAR LOS MENSAJES DE ENVIO DE MENSAJES CORRESPONDIENTES
    return;
}

int gestionar_envio_mensajes(struct info_usuario datos_usuario, struct mensaje mensaje_a_enviar){
    // Aquí se prepara el mensaje a enviar y se envía al cliente
    int sd_cliente = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd_cliente < 0){
        // No se ha podido crear el socket
        return -1;
    }

    struct sockaddr_in direccion_cliente;
    direccion_cliente.sin_family = AF_INET;
    direccion_cliente.sin_port = htons(datos_usuario.puerto_escucha_cliente);
    inet_pton(AF_INET, datos_usuario.ip, &direccion_cliente.sin_addr);

    if(connect(sd_cliente,(struct sockaddr*) &direccion_cliente,sizeof(direccion_cliente)) < 0){
        // No se ha podido conectar con el cliente
        close(sd_cliente);
        printf("Error en el connect\n");
        printf("  IP='%s' puerto=%d\n", datos_usuario.ip, datos_usuario.puerto_escucha_cliente);
        return -1;
    }

    // Enviamos el mensaje al cliente
    char buffer_envio[BUFFER_SIZE] = "SEND_MESSAGE";
    if (sendMessage(sd_cliente, buffer_envio, 14) < 0){
        // No se ha podido enviar el mensaje
        close(sd_cliente);
        return -1;
    }
    if (sendMessage(sd_cliente, mensaje_a_enviar.usuario_origen, strlen(mensaje_a_enviar.usuario_origen) + 1) < 0){
        // No se ha podido enviar el mensaje
        close(sd_cliente);
        return -1;
    }
    char str_id [4];
    sprintf(str_id, "%03d", mensaje_a_enviar.id);
    if (sendMessage(sd_cliente, str_id, 4) < 0){
        // No se ha podido enviar el mensaje
        close(sd_cliente);
        return -1;
    }
    sprintf(buffer_envio, "%s", mensaje_a_enviar.texto_mensaje);
    if (sendMessage(sd_cliente, buffer_envio, strlen(buffer_envio) + 1) < 0){
        // No se ha podido enviar el mensaje
        close(sd_cliente);
        return -1;
    }
    close(sd_cliente);
    return 0;
}