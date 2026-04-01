#include <unistd.h>

int sendMessage(int socket, char *buffer, int len);
int recvMessage(int socket, char *buffer, int len);
ssize_t readLine(int fd, void *buffer, size_t n);
ssize_t readFull(int fd, void *buffer, size_t len);
ssize_t writeFull(int fd, void *buffer, size_t len);

