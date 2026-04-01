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

/* PREGUNTAR QUE CODIGO DE ERROR PONER SI ALGUN MENSAJE EN ESPERA NO SE CONSIGUE ENVIAR
EN LA FUNCION GESTIONAR_CONNECT*/







void gestionar_register(struct peticion datos_recibidos){

    // A continuación el servidor debe obtener el nombre de usuario a registrar
    char buffer_recepcion[BUFFER_SIZE];

    // El codigo de error es un unico byte
    char codigo;
    int sd = datos_recibidos.socket_cliente;
    // Obtenemos el usuario
    if(readLine(sd,buffer_recepcion,BUFFER_SIZE) > 0){
        // Hemos obtenido texto, asumimos que es el usuario a registrar
        
        if(mkdir("clientes",0700) == -1){
            
            if(errno != EEXIST){
                // Hubo un error que no fue debido a que el directorio ya existía
                
                // Enviamos el mensaje de error y finalizamos la ejecución
                codigo = 2;
                // Enviamos el codigo a destino
                sendMessage(sd,&codigo,1);
                // Mostramos el mensaje de error
                printf("s> REGISTER %s FAIL\n", buffer_recepcion);
                return;
            }
            // Si errno == EEXIST podemos intentar crear el usuario
        }
        
        // Verificamos que no exista ningún usuario con ese nombre. Para ello accedemos al directorio
        // y comprobamos si el archivo ya existe
        
        // Formamos la ruta del archivo para ese usuario
        char ruta[BUFFER_SIZE + 10];
        sprintf(ruta,"clientes/%s",buffer_recepcion);

        int fd = open(ruta,O_CREAT | O_EXCL | O_WRONLY, 0644);

        if(fd < 0){
            // El archivo ya existe
            
            // Enviamos el codigo de error y finalizamos la ejecución
            codigo = 1;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error
            printf("s> REGISTER %s FAIL\n", buffer_recepcion);
            return;
        }

        // Se ha podido crear el archivo por lo que ahora escribimos la información del usuario
        struct info_usuario datos_usuario;
        memset(&datos_usuario,0,sizeof(datos_usuario));
        // Copiamos el nombre del usuario
        strncpy(datos_usuario.nombre_cliente,buffer_recepcion,255);
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
            printf("s> REGISTER %s FAIL\n", buffer_recepcion);
            close(fd);
            return;
        }
        
        if(writeFull(fd,(char* ) &datos_usuario,sizeof(datos_usuario)) != 0){
            codigo = 2;
            sendMessage(sd,&codigo,1);
            printf("s> REGISTER %s FAIL\n", buffer_recepcion);
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
        printf("s> REGISTER %s OK\n",buffer_recepcion);

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
    char buffer_recepcion[BUFFER_SIZE];

    // El codigo de error es un unico byte
    char codigo;
    int sd = datos_recibidos.socket_cliente;
    // Obtenemos el usuario
    if(readLine(sd,buffer_recepcion,BUFFER_SIZE) > 0){
        
        // Verificamos que exista un usuario con ese nombre. Para ello accedemos al directorio
        // y comprobamos si el archivo ya existe
        
        // Formamos la ruta del archivo para ese usuario
        char ruta[BUFFER_SIZE + 10];
        sprintf(ruta,"clientes/%s",buffer_recepcion);

        int fd = open(ruta,O_RDONLY);

        if(fd < 0){
            // El usuario no existe, se envia código 1 al cliente
            codigo = 1;
            sendMessage(sd, &codigo, 1);
            // Creamos el mensaje de error
            printf("s> UNREGISTER %s FAIL\n", buffer_recepcion);
            return;
        }

        // Añadimos LOCK_NB para que si se está usando otro lock por otro proceso, entonces no espere
        // y que devuelva -1
        if(flock(fd,LOCK_EX | LOCK_NB) == -1){
            // No se pudo bloquear el file porque ya está siendo usado, devolvemos error
            codigo = 2;
            sendMessage(sd, &codigo,1);
            printf("s> UNREGISTER %s FAIL\n", buffer_recepcion);
            close(fd);
            return;

        }

        // Aquí ya tenemos el archivo bloqueado por lo que lo podemos borrar

        if(remove(ruta) == 0){
            // Se borró de forma exitosa
            codigo = 0;
            sendMessage(sd,&codigo,1);
            printf("s> UNREGISTER %s OK\n", buffer_recepcion);
            

        } else{
             codigo = 2;
            sendMessage(sd, &codigo,1);
            printf("s> UNREGISTER %s FAIL\n", buffer_recepcion);
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
        struct mensaje mensaje_obtenido;
        struct mensaje mensajes_fallidos[BUFFER_SIZE];

        int num_fallidos = 0;

        while(readFull(fd,&mensaje_obtenido,sizeof(struct mensaje)) == 0){
            // Hemos obtenido un mensaje, lo intentamos enviar

            if(gestionar_envio_mensaje(datos_usuario,mensaje_obtenido) == -1){
                // Guardamos el mensaje que no se consiguió enviar y aumentamos el contador
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

        for(int i = 0; i < num_fallidos; i++){
            if(writeFull(fd,&mensajes_fallidos[i],sizeof(struct mensaje)) != 0){
                // Hubo un error en reescribir dicho mensaje
                // codigo = 3;
                // sendMessage(sd, &codigo, 1);
                // // Creamos el mensaje de error, no se ha podido leer el nombre de usuario
                // printf("s> CONNECT %s FAIL\n",nombre_usuario);
                // flock(fd, LOCK_UN);
                // close(fd);
                // return;
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

int gestionar_envio_mensaje(struct info_usuario datos_source,struct mensaje mensaje_a_enviar){
    // nombre del cliente está en el struct datos_usuario
    // el segundo parámetro es el mensaje a enviar, se devuelve -1 si no se consigue enviar el mensaje
    // ESTA FUNCION SE ENCARGARA DE MOSTRAR LOS MENSAJES DE ENVIO DE MENSAJES CORRESPONDIENTES
}