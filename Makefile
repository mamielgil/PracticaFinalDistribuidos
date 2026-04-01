
CC = gcc

CFLAGS = -Wall -fPIC -pthread -Iinclude

LDLIBS = -pthread


all: servidor


servidor: servidor.o gestionar_peticiones.o lines.o
	$(CC) -o $@ $^ $(LDLIBS)

servidor.o: src/servidor.c include/gestionar_peticiones.h include/lines.h
	$(CC) $(CFLAGS) -c $< -o $@

gestionar_peticiones.o: src/gestionar_peticiones.c include/gestionar_peticiones.h include/lines.h
	$(CC) $(CFLAGS) -c $< -o $@

lines.o: src/lines.c include/lines.h
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar
clean:
	rm -f *.o servidor