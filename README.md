**Con el proyecto, se ha proporcionado un archivo run.sh**. 

Este archivo crea 5 terminales. El objetivo es facilitar la simulación del sistema. Se asocia una terminal al servidor que gestiona las peticiones, otra terminal al servidor web que normaliza los mensajes y otra terminal al servidor RPC que registra cada una de las peticiones realizadas. Por otro lado, se abren dos terminales que ejecutan el archivo client.py. A través de estas terminales, se pueden enviar peticiones para simular el uso del sistema. Si se considera conveniente, se pueden abrir más terminales para incluir más clientes en la ejecución. Este archivo .sh permite comprobar el flujo utilizando las variables **IP_SERVER_PETICIONES** y
**PUERTO_SERVER_PETICIONES**. Estas variables modifican donde escucha y se ejecuta el servidor.

**ALTERNATIVAMENTE SE PUEDEN REALIZAR LAS INSTRUCCIONES DETALLAS A CONTINUACIÓN**


# Instrucciones para realizar la compilación

Primero, para compilar todos los archivos y generar los ejecutables necesarios, es necesario ejecutar el siguiente comando en el directorio más externo:

```bash
make
```
Una vez ejecutado el make, se deben realizar los siguientes comandos en distintas terminales:

**Terminal 1**

1. Ejecutamos el servidor web

```bash
cd src
cd servicio_web_final
python3 servicio_web_espacios.py
```

**Terminal 2**

2. Ejecutamos el servidor RPC

```bash
cd src
cd RPC_server
./server-rpc
```

**Terminal 3**

3. Ejecutamos el servidor que gestiona las distintas peticiones asociadas a servicios del sistema(REGISTER, CONNECT...)

```bash
LOG_RPC_IP=localhost ./servidor -p 5000
```

**Terminal 4,5, 6...**

4. Ejecutamos el número de terminales deseadas, cada una de ellas asociadas a un cliente concreto.

```bash
cd src
python3 client.py -s localhost -p 5000
```

En cada terminal cliente se podrán hacer las instrucciones deseadas. Estas serán gestionadas por el servidor que procesa las posibles instrucciones que se pueden pedir al sistema.

