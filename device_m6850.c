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
#include "device_m6850.h"
#include "io_com_udp.h"

#define BUFFER_SIZE 1

struct m6850_port
{
  uint8_t ctrl; /* Control Register */
  uint8_t status; /* Status Register */
  uint8_t RDR; /* Receive Data Register */
  uint8_t TDR; /* Transmit Data Register */
};

uint8_t buffer_put[BUFFER_SIZE];
uint8_t buffer_get[BUFFER_SIZE];
uint8_t index_put = 0;
uint8_t index_get = 0;

int socket_rx, socket_tx;

#define RX_LOCAL_PORT 4000
#define TX_LOCAL_PORT 4001
#define TX_DST_PORT 5000


#define TRANSMIT_DATA_REGISTER 1    
#define RECEIVE_DATA_REGISTER 1
#define CONTROL_REGISTER 0
#define STATUS_REGISTER 0

#define SER_CTL_RESET   0x03   /* CR1:0=11 - Reset device */

#define SER_STAT_READOK  0x1
#define SER_STAT_WRITEOK 0x2
#define SER_STAT_TXEMPTY 0x2

void m6850_update_com();
void m6850_init_com();

void m6850_update (struct hw_device *dev)
{
  m6850_update_com();
  struct m6850_port *port = (struct m6850_port *)dev->priv;
  if(((port->status & 0x01) == 0x00) && (index_put > 0))
  {
    /* A byte is pending to be received */
    port->status |= 0x01; /* RDRF = 1 */
    port->RDR = buffer_put[index_put-1];
    if(port->ctrl & 0x80 == 0x80) /* Receive Interrupt is enabled */
      port->status |= 0x80; /* IRQ  bit set*/
    index_put--;
  }
  if(((port->status & 0x02) == 0x00) && (index_get < BUFFER_SIZE))
  {
    /* A byte is pending to be sent */
    buffer_get[index_get] = port->TDR;
    index_get++;
    port->status |= 0x02; /* TDRE = 1 */
    if(((port->ctrl & 0x10) == 0x10) && ((port->ctrl | 0x20) == 0x20)) /* Transmit Interrupt is enabled */
      port->status |= 0x80; /* IRQ  bit set*/
  }
}

uint8_t m6850_read (struct hw_device *dev, unsigned long addr)
{
  struct m6850_port *port = (struct m6850_port *)dev->priv;
  switch (addr)
  {
    case STATUS_REGISTER:
    {
      return port->status;
    }
    case RECEIVE_DATA_REGISTER:
    {
      port->status &=~(0x01); /* Clear RDRF */
      if(port->ctrl & 0x80 == 0x80) /* Receive Interrupt is enabled */
        port->status &=~(0x80); /* Clear IRQ */
      return port->RDR;
    }
      
  }
}

void m6850_master_reset(struct m6850_port *port)
{
  port->status = 0x02;
  port->RDR = 0;
  port->TDR = 0;
}

void m6850_write (struct hw_device *dev, unsigned long addr, uint8_t val)
{
  struct m6850_port *port = (struct m6850_port *)dev->priv;
  switch (addr) 
  {
    case CONTROL_REGISTER:
    {
      port->ctrl = val;
      if (val & 0x03 == 0x03)
        m6850_master_reset(port);
      break;
    }
    case TRANSMIT_DATA_REGISTER:
    {
      port->TDR = val;
      port->status &=~(0x02); /* Clear TDRE */
      if(((port->ctrl & 0x10) == 0x10) && ((port->ctrl | 0x20) == 0x20)) /* Transmit Interrupt is enabled */
        port->status &=~(0x80); /* Clear IRQ */
      break;
    }
  }
}

void m6850_reset (struct hw_device *dev)
{
  struct m6850_port *port = (struct m6850_port *)dev->priv;
  port->ctrl = 0;
  m6850_master_reset(port);

}

void m6850_dump (struct hw_device *dev)
{
  struct m6850_port *port = (struct m6850_port *)dev->priv;
  printf("(dbg) -- M6850 registers --\n");
  printf("(dbg) CR: 0x%02X  SR: 0x%02X\n", port->ctrl, port->status);
  printf("(dbg) RDR: 0x%02X  TDR: 0x%02X\n", port->RDR, port->TDR);
}

uint8_t m6850_irq_pending(struct hw_device *dev)
{
    struct m6850_port *port = (struct m6850_port *)dev->priv;
    return port->status & 0x80;
}

struct hw_class m6850_class =
  {
    .name = "m6850",
    .readonly = 0,
    .reset = m6850_reset,
    .read = m6850_read,
    .write = m6850_write,
    .update = m6850_update,
    .dump = m6850_dump,
    .check_interrupt = m6850_irq_pending,
  };


struct hw_device* m6850_create (unsigned long size)
{
  struct m6850_port *port = malloc (sizeof (struct m6850_port));
  m6850_init_com();
  return device_create (&m6850_class, size, port);
}

/* User functions */

uint8_t m6850_getchar()
{
  uint8_t val = 0xFF;
  if(index_get > 0)
  {
    val = buffer_get[index_get-1];
    index_get--;
  }
  return val;
}

void m6850_putchar(uint8_t val)
{
  if(index_put < BUFFER_SIZE)
  {
    buffer_put[index_put] = val;
    index_put++;
  }
}

uint8_t m6850_kbhit()
{
  return index_get;
}

/* Return 0 if there is space left */
uint8_t m6850_putready()
{
  if(index_put > BUFFER_SIZE-1)
    return 1;
  else
    return 0;
}

void m6850_init_com()
{
  socket_rx = udp_com_socket_create (RX_LOCAL_PORT);
  socket_tx = udp_com_socket_create (TX_LOCAL_PORT);
}

void m6850_update_com()
{
  int rc;
  uint8_t val;
  if(m6850_kbhit())
  {
    val = m6850_getchar();
    udp_com_socket_send (socket_tx, TX_DST_PORT, &val, sizeof (val));
  }
  if(m6850_putready() == 0)
  {
    rc = udp_com_socket_receive (socket_rx, 0, &val, sizeof (val));
    if(rc == 1)
        m6850_putchar(val);
  }
}

