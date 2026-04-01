# Compilador
CC = gcc

CFLAGS = -Wall -fPIC -pthread

LDLIBS = -pthread

# Regla por defecto
all:servidor

lines.o: lines.c
	$(CC) $(CFLAGS) -c $< -o $@

servidor: servidor.o lines.o
	$(CC) -o $@ $^ $(LDLIBS)

servidor.o: servidor.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar
clean:
	rm -f *.o *.so servidor