#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define UART_SRC_CLIENT_PORT 8001
#define UART_DST_SERVER_PORT 9000
#define UART_SRC_SERVER_PORT 7401

void udp_socket_error (void)
{
	abort ();
}

int udp_socket_create (int port)
{
	int rc;
	int s;
	struct sockaddr_in myaddr;

	s = socket (PF_INET, SOCK_DGRAM, 0);
	if (s < 0)
	{
		fprintf (stderr, "could not open socket, errno=%d\n", errno);
		udp_socket_error ();
		return s;
	}

	rc = fcntl (s, F_SETFL, O_NONBLOCK);
	if (rc < 0)
	{
		fprintf (stderr, "could not set nonblocking, errno=%d\n", errno);
		udp_socket_error ();
		return rc;
	}

	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons (port);
	myaddr.sin_addr.s_addr = INADDR_ANY;
	rc = bind (s, (struct sockaddr *)&myaddr, sizeof (myaddr));
	if (rc < 0)
	{
		fprintf (stderr, "could not bind socket, errno=%d\n", errno);
		udp_socket_error ();
		return rc;
	}

	return s;
}

int udp_socket_send (int s, int dstport, const void *data, socklen_t len)
{
	int rc;
	struct sockaddr_in to;

	to.sin_family = AF_INET;
	to.sin_port = htons (dstport);
	to.sin_addr.s_addr = inet_addr ("127.0.0.1");
	rc = sendto (s, data, len, 0, (struct sockaddr *)&to, sizeof (to));
	if ((rc < 0) && (errno != EAGAIN))
	{
		fprintf (stderr, "could not send, errno=%d\n", errno);
		udp_socket_error ();
	}
	return rc;
}

int udp_socket_receive (int s, int dstport, void *data, socklen_t len)
{
	int rc;
	struct sockaddr_in from;

	rc = recvfrom (s, data, len, 0, (struct sockaddr *)&from, &len);
	if ((rc < 0) && (errno != EAGAIN))
	{
		fprintf (stderr, "could not receive, errno=%d\n", errno);
		udp_socket_error ();
	}
	return rc;
}

int udp_socket_close (int s)
{
	close (s);
	return 0;
}

/* Non-blocking check for input character. If
 *   true, retrieve character using kbchar()
 */
int kbhit(void)
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int kbchar(void)
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}

int main (int argc, char *argv[])
{
	int server, client;
	char csw_file[8192];
	size_t len;
	char sendbuf[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	char recvbuf[8];
	char ch;
	FILE *fp;
	char array[2];
	int i;
	int update_in_progress = 0;
	server = udp_socket_create (UART_SRC_SERVER_PORT);
	client = udp_socket_create (UART_SRC_CLIENT_PORT);
	printf ("CSW Loader 1.0.0\n");
	fp = fopen(argv[1], "rb");
	if(fp != NULL)
	{
		len = fread(csw_file, sizeof(char), 8192, fp);
		printf("CSW %s of %d bytes loaded\n",argv[1],len);
		fclose(fp);
	}
	else
	{
		printf("Warning: CSW not found!\n");
	}
	while(1)
	{
		/*
		if(kbhit())	
		{
				ch = kbchar();
				printf("%c", ch);
				if(ch != 10)
				{
					udp_socket_send (client, UART_DST_SERVER_PORT, &ch, sizeof (ch));
				}
		}
		*/
		
		if(udp_socket_receive (server, 0, recvbuf, sizeof (recvbuf)) > 0)
		{
			printf("%c", recvbuf[0]);
			fflush(stdout);
			if(recvbuf[0] == '?' && len != 0)
			{
				array[0]='!';
				udp_socket_send (client, UART_DST_SERVER_PORT, &array[0], sizeof (char));
				for(i=0;i<64;i++)
				{
					udp_socket_send (client, UART_DST_SERVER_PORT, &csw_file[i], sizeof (char));
				}
				update_in_progress++;
			}
			else if(recvbuf[0] == '#' && len != 0 && update_in_progress >= 1)
			{
				for(i=64*update_in_progress;i<64*(update_in_progress+1);i++)
				{
					udp_socket_send (client, UART_DST_SERVER_PORT, &csw_file[i], sizeof (char));
				}
				update_in_progress++;
				if(update_in_progress == 64)
				{
					update_in_progress = 0;
				}
			}
		}
	}
	udp_socket_close(server);
	udp_socket_close(client);
}