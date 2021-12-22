#ifndef DEVICE_MC6850_H
#define DEVICE_MC6850_H

struct mc6850_port
{
  unsigned int ctrl;
  unsigned int status;
  int fin;
  int fout;
};

/* The I/O registers exposed by this driver */
#define SER_CTL_STATUS   0     /* Control (write) and status (read) */
#define SER_DATA         1     /* Data input/output */

#define SER_CTL_RESET   0x03   /* CR1:0=11 - Reset device */

#define SER_STAT_READOK  0x1
#define SER_STAT_WRITEOK 0x2
#define SER_STAT_TXEMPTY 0x2

struct hw_device* mc6850_create (void);



#endif
