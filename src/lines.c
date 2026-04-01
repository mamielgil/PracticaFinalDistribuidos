#include "lines.h"
#include <unistd.h>
#include <errno.h>

int sendMessage(int socket, char * buffer, int len)
{
	int r;
	int l = len;
		

	do {	
		r = write(socket, buffer, l);
		l = l -r;
		buffer = buffer + r;
	} while ((l>0) && (r>=0));
	
	if (r < 0)
		return (-1);   /* fail */
	else
		return(0);	/* full length has been sent */
}

int recvMessage(int socket, char *buffer, int len)
{
	int r;
	int l = len;
		

	do {	
		r = read(socket, buffer, l);
		l = l -r ;
		buffer = buffer + r;
	} while ((l>0) && (r>=0));
	
	if (r < 0)
		return (-1);   /* fallo */
	else
		return(0);	/* full length has been receive */
}



ssize_t readLine(int fd, void *buffer, size_t n)
{
	ssize_t numRead;  /* num of bytes fetched by last read() */
	size_t totRead;	  /* total bytes read so far */
	char *buf;
	char ch;


	if (n <= 0 || buffer == NULL) { 
		errno = EINVAL;
		return -1; 
	}

	buf = buffer;
	totRead = 0;
	
	for (;;) {
        	numRead = read(fd, &ch, 1);	/* read a byte */

        	if (numRead == -1) {	
            		if (errno == EINTR)	/* interrupted -> restart read() */
                		continue;
            	else
			return -1;		/* some other error */
        	} else if (numRead == 0) {	/* EOF */
            		if (totRead == 0)	/* no byres read; return 0 */
                		return 0;
			else
                		break;
        	} else {			/* numRead must be 1 if we get here*/
            		if (ch == '\n')
                		break;
            		if (ch == '\0')
                		break;
            		if (totRead < n - 1) {		/* discard > (n-1) bytes */
				totRead++;
				*buf++ = ch; 
			}
		} 
	}
	
	*buf = '\0';
    	return totRead;
}


ssize_t writeFull(int fd, void *buffer, size_t len) {
    int r;
    int l = len;
    char *buf = (char *)buffer;

    while (l > 0) {
        r = write(fd, buf, l);
        
        if (r < 0) {
            return -1; // Fallo crítico
        } else if (r == 0) {
            // No se escribió nada
            return -1; 
        }
        
        l = l - r;
        buf = buf + r;
    }
    return 0; // Éxito: se escribieron todos los bytes
}

ssize_t readFull(int fd, void *buffer, size_t len) {
    int r;
    int l = len;
    char *buf = (char *)buffer;

    while (l > 0) {
        r = read(fd, buf, l);
        
        if (r < 0) {
            return -1; // Fallo crítico
        } else if (r == 0) {
            // Si llegamos aquí y l > 0, significa que el archivo se acabó a medias.
            return -1; 
        }
        
        l = l - r;
        buf = buf + r;
    }
    return 0; // Éxito: se leyeron todos los bytes
}
