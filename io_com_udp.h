#ifndef IO_COM_UDP_H
#define IO_COM_UDP_H

#include <sys/socket.h>

int udp_com_socket_close (int s);
int udp_com_socket_receive (int s, int dstport, void *data, socklen_t len);
int udp_com_socket_send (int s, int dstport, const void *data, socklen_t len);
int udp_com_socket_create (int port);

#endif