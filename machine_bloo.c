#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>

#include "machine.h"
#include "device.h"
#include "device_m6850.h"
#include "device_m6840.h"
#include "device_ram.h"
#include "device_rom.h"
#include "machine_bloo.h"




/********************************************************************
 * bloo
 *
 * 48KByte of RAM at $0000
 * 6850 at $C000, $C001
 * 6840 at $C400, ...
 * 8KByte of ROM at $E000
 ********************************************************************/


 
 void bloo_init ()
{
   	struct hw_device *uart;
	struct hw_device *ptm;
	struct hw_device *rom;
	struct hw_device *ram;

    /* 48K RAM from 0000 to BFFF */
    ram = ram_create(BLOO_RAM_SIZE);
	machine_attach_device(ram);
	machine_map_device (ram , 0, BLOO_RAM_BASE, BLOO_RAM_SIZE, MAP_READWRITE );

	/* $C000 and $C001 for 6850 */
	uart = m6850_create(BLOO_UART_SIZE);
	machine_attach_device(uart);
    machine_map_device (uart, 0, BLOO_UART_BASE, BLOO_UART_SIZE, MAP_READWRITE);
	machine_attach_irq (uart);

	/* From $C400 to  $C4xx for 6840 */
	ptm = m6840_create(BLOO_PTM_SIZE);
	machine_attach_device(ptm);
    machine_map_device (ptm, 0, BLOO_PTM_BASE, BLOO_PTM_SIZE, MAP_READWRITE);
	machine_attach_irq (ptm);
	
	/* 8K ROM from E000 to FFFF */
    rom = rom_create (BLOO_ROM_SIZE);
	machine_attach_device(rom);
    machine_map_device (rom , 0, BLOO_ROM_BASE, BLOO_ROM_SIZE, MAP_READWRITE);
}


 
machine_t bloo_machine =
{
	.name = "bloo",
	.fault = NULL,
	.init = bloo_init,
	.dump = NULL,
	.irq = NULL,
	.firq = NULL,
};
