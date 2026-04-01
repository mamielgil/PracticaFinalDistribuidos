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

# define NUM_THREADS 50


struct peticion buffer_socket[NUM_THREADS];
int n_elementos = 0;
int pos_servicio = 0;
pthread_mutex_t mi_mutex;
pthread_cond_t no_lleno;
pthread_cond_t no_vacio;
pthread_mutex_t mfin;
int fin = 0;

// Función que determina la petición que desea el cliente
void procesar_peticion(struct peticion);

// Función que gestionar la petición REGISTER
void gestionar_register(struct peticion);

void* tratar_usuario(){
   
    while(1){
        pthread_mutex_lock(&mi_mutex);

        while(n_elementos == 0){

            if(fin == 1){
                printf("s> Finalizando el servicio\n");
                pthread_mutex_unlock(&mi_mutex);
                pthread_exit(0);
            }
            pthread_cond_wait(&no_vacio,&mi_mutex);
        }
        struct peticion datos_cliente = buffer_socket[pos_servicio];
        int sd = datos_cliente.socket_cliente;
        pos_servicio = (pos_servicio + 1) % NUM_THREADS;
        n_elementos--;
        pthread_cond_signal(&no_lleno);
        pthread_mutex_unlock(&mi_mutex);


        // Procesamos la petición

        procesar_peticion(datos_cliente);

        // Cerrramos el socket al haber tratado la solicitud
        close(sd);
    }
    pthread_exit(0);
   
}

int main(int argc, char *argv[]){

    if(argc !=2){
        perror("s> Necesitas especificar el número de puerto\n");
        exit(1);
    }

    pthread_attr_t thread_config;
    pthread_t thread_pool[NUM_THREADS];
    int pos = 0;

    int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(sd < 0){
        // El socket no se creo de forma exitosa
        perror("s> Error in socket");
        exit(1);
    }
    int val = 1;

    int err = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(int));

    if( err < 0){
        // La configuración se asignó de forma errónea
        perror("s> Error en el establecimiento de la configuración");
        exit(1);
    }

    int port_number = atoi(argv[1]);

    if(port_number == 0){
        // Error al hacer el atoi
        perror("s> No se pudo obtener el puerto del servidor");
        exit(1);
    }

    struct sockaddr_in server_addr;

    bzero((char*)&server_addr, sizeof(server_addr));

    // Establecemos que usamos IPv4
    server_addr.sin_family = AF_INET;

    // Le asigna cualquier interfaz de red de nuestro dispositivo
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Transformamos el puerto a formato de red
    server_addr.sin_port = htons(port_number);

    // Asignamos al socket la configuración deseada
    err = bind(sd, (const struct sockaddr *) &server_addr, sizeof(server_addr));

    if(err == -1){
        printf("s> Error en el bind\n");
        return -1;
    }

    err = listen(sd,SOMAXCONN);

    if(err == -1){
        printf("s> Error en el listen\n");
        return -1;
    }

    // Como el servidor ya está inicializado hacemos el print correspondiente
    char ip_servidor[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(server_addr.sin_addr),ip_servidor,INET_ADDRSTRLEN);
    printf("s> init server %s:%d\n",ip_servidor,ntohs(server_addr.sin_port));

    // Hemos decidido hacer los threads detached de forma que liberan
    // sus recursos de forma automática. 
    pthread_attr_init(&thread_config);
    pthread_mutex_init(&mi_mutex,NULL);
    pthread_mutex_init(&mfin, NULL);
    pthread_cond_init(&no_lleno,NULL);
    pthread_cond_init(&no_vacio,NULL);

    // Creamos la pool de threads
    for(int i = 0; i < NUM_THREADS; i++){
        if(pthread_create(&thread_pool[i],&thread_config,tratar_usuario,NULL) != 0){
            perror("Error creando la pool de threads\n");
            return -1;
        }

    }

    while(1){
        struct peticion pet;
        struct sockaddr_in datos_conexion;
        socklen_t tamaño_conexion = sizeof(datos_conexion);
        int sc = accept(sd, (struct sockaddr*)&datos_conexion, &tamaño_conexion);

        if(sc == -1){
            printf("s> Error en el accept\n");
            continue;
        }

        pthread_mutex_lock(&mi_mutex);

        while(n_elementos == NUM_THREADS){
            // No caben más peticiones en la cola por lo que se debe de esperar
            // antes de recibir más peticiones
            pthread_cond_wait(&no_lleno,&mi_mutex);
        }

        // Guardamos la nueva peticion recibida en el buffer para que pueda ser procesado por 
        // la pool de threads

        pet.socket_cliente = sc;
        // Obtenemos la dirección IP del cliente
        inet_ntop(AF_INET,&(datos_conexion.sin_addr),pet.ip,INET_ADDRSTRLEN);
        buffer_socket[pos] = pet;

        // Aumentamos el índice pos para que la siguiente petición no sobreescriba a la recibida
        pos = (pos + 1) % NUM_THREADS;
        n_elementos++;
        pthread_cond_signal(&no_vacio);
        pthread_mutex_unlock(&mi_mutex);
        }

    pthread_mutex_lock(&mfin);
    fin = 1;
    pthread_mutex_unlock(&mfin);

    pthread_mutex_lock(&mi_mutex);
    pthread_cond_broadcast(&no_vacio);
    pthread_mutex_unlock(&mi_mutex);



    // Ahora nos aseguramos que todos los threads de la pool finalicen su ejecución

    for(int i = 0; i < NUM_THREADS; i++){
        pthread_join(thread_pool[i],NULL);

    }
    pthread_mutex_destroy(&mi_mutex);
    pthread_cond_destroy(&no_lleno);
    pthread_cond_destroy(&no_vacio);
    pthread_mutex_destroy(&mfin);
    close(sd);
}


void procesar_peticion(struct peticion datos_cliente){

    //En esta función recibimos el socket del usuario y esperamos a obtener la instrucción a realizar

    // Inicializamos un buffer donde vamos a recibir los params
    char buffer[BUFFER_SIZE];
    int sd = datos_cliente.socket_cliente;
    // Recibimos la operación a realizar
    if(readLine(sd,buffer,BUFFER_SIZE) > 0){

        // Significa que hemos leído algo
        if(strcmp(buffer,"REGISTER") == 0){
            // En este caso, realizamos la operación register
            gestionar_register(datos_cliente);
            
        }else if(strcmp(buffer, "UNREGISTER") == 0){
            // En este caso, gestionamos el unregister
            gestionar_unregister(datos_cliente);

        }else if(strcmp(buffer,"CONNECT") == 0){
            // Ahora lidiamos con el connect
            gestionar_connect(datos_cliente);
        }
    }


}

