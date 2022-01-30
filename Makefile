SRCS=m6809.c \
	device_rom.c device_ram.c logging.c\
	device_mc6850.c device.c debugger.c\
	monitor.c  symtab.c  command.c simulator.c main.c\
	bus_access.c machine.c machine_bloo.c \
	io_com_udp.c io_file.c utils_time.c

#CFLAGS=-DHAVE_READLINE -DHAVE_TERMIOS
LDFLAGS=

#LIBS=-lreadline

CC=gcc
LD=gcc

HDRS=$(wildcard *.h)
OBJS=$(subst .c,.o,$(SRCS))

.c.o: $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY:	all info clean

all: m6809-run

info:
	@echo Sources $(SRCS)
	@echo Headers $(HDRS)
	@echo Objects $(OBJS)

m6809-run: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LIBS)


clean:
	rm -f m6809-run $(OBJS)

