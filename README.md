# Instrucciones para realizar la compilación

Primero, para compilar todos los archivos y generar los ejecutables necesarios, es necesario ejecutar el siguiente comando en el directorio más externo:

```C
make
```
Una vez ejecutado el make, se deben realizar los siguientes comandos en distintas terminales:

**Terminal 1**

1. Ejecutamos el servidor web

```C
cd src
cd servicio_web_final
python3 servicio_web_espacios.py
```

**Terminal 2**

2. Ejecutamos el servidor RPC

```C
cd src
cd RPC_server
./server-rpc
```

**Terminal 3**
3. Ejecutamos el servidor que gestiona las distintas peticiones asociadas a servicios del sistema(REGISTER, CONNECT...)

```C
LOG_RPC_IP=localhost ./servidor -p 5000
```

**Terminal 4,5, 6...**

4. Ejecutamos el número de terminales deseadas, cada una de ellas asociadas a un cliente concreto.

```C
cd src
python3 client.py -s localhost -p 5000
```

En cada terminal cliente se podrán hacer las instrucciones deseadas que serán gestionadas por el servidor que gestiona las peticiones del sistema.