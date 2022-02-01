#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include "types.h"
#include "device.h"
#include "device_mc6850.h"
#include "io_com_udp.h"

#define UART_SRC_CLIENT_PORT 8000
#define UART_DST_SERVER_PORT 7401
#define UART_SRC_SERVER_PORT 9000

int uart_server, uart_client;

/* Emulate a serial port.  Basically this driver can be used for any byte-at-a-time
   input/output interface. */


void mc6850_update (struct mc6850_port *port)
{
  fd_set infds, outfds;
  struct timeval timeout;
  int rc;
  int max_sd;
	if(port->fin > port->fout)    
      max_sd = (port->fin + 1);
	else  
      max_sd = (port->fout + 1);
  FD_ZERO (&infds);
  FD_SET (port->fin, &infds);
  FD_ZERO (&outfds);
  FD_SET (port->fout, &outfds);
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  rc = select (max_sd, &infds, &outfds, NULL, &timeout);
  if (FD_ISSET (port->fin, &infds))
    port->status |= SER_STAT_READOK;
  else
    port->status &= ~SER_STAT_READOK;
  if (FD_ISSET (port->fout, &outfds))
    port->status |= SER_STAT_WRITEOK;
  else
    port->status &= ~SER_STAT_WRITEOK;
}

uint8_t mc6850_read (struct hw_device *dev, unsigned long addr)
{
  struct mc6850_port *port = (struct mc6850_port *)dev->priv;
  int retval;
  mc6850_update (port);
  switch (addr)
    {
    case SER_DATA:
      {
	uint8_t val;
	if (!(port->status & SER_STAT_READOK))
	  return 0xFF;
	//retval = read (port->fin, &val, 1);
	udp_com_socket_receive (uart_server, 0, &val, sizeof (val));
	//assert(retval != -1);
	return val;
      }
    case SER_CTL_STATUS:
      return port->status;
    }
}

void mc6850_write (struct hw_device *dev, unsigned long addr, uint8_t val)
{
  struct mc6850_port *port = (struct mc6850_port *)dev->priv;
  int retval;

  switch (addr) {
  case SER_DATA:
    {
      uint8_t v = val;
      //retval = write (port->fout, &v, 1);
	  //transmit data register
      udp_com_socket_send (uart_client, UART_DST_SERVER_PORT, &val, sizeof (val));
      //assert(retval != -1);
      break;
    }
  case SER_CTL_STATUS:
    port->ctrl = val;
    break;
  }
}

void mc6850_reset (struct hw_device *dev)
{
  struct mc6850_port *port = (struct mc6850_port *)dev->priv;
  port->ctrl = 0;
  port->status = SER_STAT_TXEMPTY;;
}

struct hw_class mc6850_class =
  {
    .name = "mc6850",
    .readonly = 0,
    .reset = mc6850_reset,
    .read = mc6850_read,
    .write = mc6850_write,
    .dump = NULL,
    .check_interrupt = NULL,
  };


struct hw_device* mc6850_create (unsigned long size)
{
  struct mc6850_port *port = malloc (sizeof (struct mc6850_port));
  uart_server = udp_com_socket_create (UART_SRC_SERVER_PORT);
  uart_client = udp_com_socket_create (UART_SRC_CLIENT_PORT);
  port->fin = uart_server;
  port->fout = uart_client;
  return device_create (&mc6850_class, size, port);
}
