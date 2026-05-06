#include <netinet/in.h>
# define BUFFER_SIZE 1024

struct mensaje {
    // Guardamos en un struct los datos que caracterizan a un mensaje
    unsigned int id;
    // Almacenamos el usuario que envió dicho mensaje
    char usuario_origen [256];
    char texto_mensaje [BUFFER_SIZE];
    char nombre_fichero [BUFFER_SIZE];

};
struct info_usuario {
    // Asumimos que el tamaño máximo es de 256 chars
    char nombre_cliente[256];
    // 0 es que está offline y 1 es que está conectado
    int estado;
    int ultimo_id;

    // IP del usuario
    char ip[INET_ADDRSTRLEN];

    // Puerto donde el proceso cliente escucha y se le pueden enviar mensajes
    int puerto_escucha_cliente;
};


struct peticion {
    int socket_cliente;
    char ip[INET_ADDRSTRLEN];
};

void gestionar_register(struct peticion datos_recibidos);
void gestionar_unregister(struct peticion datos_recibidos);
void gestionar_connect(struct peticion datos_recibidos);
void gestionar_disconnect(struct peticion datos_recibidos);
void gestionar_users(struct peticion datos_recibidos);
char** leer_users(int * num_users_conn);
int gestionar_envio_mensajes(struct info_usuario datos_source,struct mensaje mensaje_a_enviar);
void gestionar_mensajes(struct peticion);