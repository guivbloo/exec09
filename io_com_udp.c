#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>



void udp_com_socket_error (void)
{
	abort ();
}

int udp_com_socket_create (int port)
{
	int rc;
	int s;
	struct sockaddr_in myaddr;

	s = socket (PF_INET, SOCK_DGRAM, 0);
	if (s < 0)
	{
		fprintf (stderr, "could not open socket, errno=%d\n", errno);
		udp_com_socket_error ();
		return s;
	}

	rc = fcntl (s, F_SETFL, O_NONBLOCK);
	if (rc < 0)
	{
		fprintf (stderr, "could not set nonblocking, errno=%d\n", errno);
		udp_com_socket_error ();
		return rc;
	}

	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons (port);
	myaddr.sin_addr.s_addr = INADDR_ANY;
	rc = bind (s, (struct sockaddr *)&myaddr, sizeof (myaddr));
	if (rc < 0)
	{
		fprintf (stderr, "could not bind socket, errno=%d\n", errno);
		udp_com_socket_error ();
		return rc;
	}

	return s;
}

int udp_com_socket_send (int s, int dstport, const void *data, socklen_t len)
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
		udp_com_socket_error ();
	}
	return rc;
}

int udp_com_socket_receive (int s, int dstport, void *data, socklen_t len)
{
	int rc;
	struct sockaddr_in from;

	rc = recvfrom (s, data, len, 0, (struct sockaddr *)&from, &len);
	if ((rc < 0) && (errno != EAGAIN))
	{
		fprintf (stderr, "could not receive, errno=%d\n", errno);
		udp_com_socket_error ();
	}
	return rc;
}

int udp_com_socket_close (int s)
{
	close (s);
	return 0;
}