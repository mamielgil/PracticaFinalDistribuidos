
CC = gcc

CFLAGS = -Wall -fPIC -pthread -Iinclude

LDLIBS = -pthread


all: servidor make_rpc

# Llamamos al makefile que compila el servidor RPC
make_rpc:
	$(MAKE) -C ./src/RPC_server all

# Permite limpiar el directorio del servidor RPC
clean_rpc:
	$(MAKE) -C ./src/RPC_server clean

servidor: servidor.o gestionar_peticiones.o lines.o
	$(CC) -o $@ $^ $(LDLIBS)

servidor.o: src/servidor.c include/gestionar_peticiones.h include/lines.h
	$(CC) $(CFLAGS) -c $< -o $@


gestionar_peticiones.o: src/gestionar_peticiones.c include/gestionar_peticiones.h include/lines.h
	$(CC) $(CFLAGS) -c $< -o $@

lines.o: src/lines.c include/lines.h
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar
clean: clean_rpc
	rm -f *.o servidor