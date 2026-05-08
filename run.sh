#!/bin/bash

make clean
make



IP_SERVER_PETICIONES="localhost"
PUERTO_SERVER_PETICIONES="5000"

# Servidor web
gnome-terminal -- bash -c "cd 'src/servicio_web_final' && python3 servicio_web_espacios.py; exec bash"

# Servidor RPC
gnome-terminal -- bash -c "cd 'src/RPC_server' && ./server-rpc; exec bash"

# Servidor de peticiones
gnome-terminal -- bash -c "LOG_RPC_IP=$IP_SERVER_PETICIONES ./servidor -p $PUERTO_SERVER_PETICIONES; exec bash"

# Cliente 1
gnome-terminal -- bash -c "cd 'src' && python3 client.py -s $IP_SERVER_PETICIONES -p $PUERTO_SERVER_PETICIONES; exec bash"

# Cliente 2
gnome-terminal -- bash -c "cd 'src' && python3 client.py -s $IP_SERVER_PETICIONES -p $PUERTO_SERVER_PETICIONES; exec bash"