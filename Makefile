SRCS=m6809.c \
	device_rom.c device_ram.c\
	device_m6850.c device_m6840.c device.c \
	monitor.c bus_access.c \
	symtab.c  simulator.c cli_monitor.c main.c\
	machine.c machine_bloo.c gui_monitor.c \
	io_file.c utils_time.c

#CFLAGS=-DHAVE_READLINE -DHAVE_TERMIOS
LDFLAGS=-lc++
CFLAGS=-std=c11

#LIBS=-lreadline

CC=gcc
LD=gcc

HDRS=$(wildcard *.h)
OBJS=$(subst .c,.o,$(SRCS))
LIBS= ../dcimgui/build/libimgui.a

.c.o: $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY:	all info clean

all: m6809-run

info:
	@echo Sources $(SRCS)
	@echo Headers $(HDRS)
	@echo Objects $(OBJS)

m6809-run: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)


clean:
	rm -f m6809-run $(OBJS)

