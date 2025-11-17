#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>

#include "machine.h"
#include "device.h"
#include "device_m6850.h"
#include "device_m6840.h"
#include "device_ram.h"
#include "device_rom.h"
#include "device_tms9918.h"
#include "machine_video_test.h"
#include "cimgui.h"




/********************************************************************
 * video_test machine architecture:
 *
 * 48KByte of RAM at $0000
 * 6850 at $C000, $C001
 * 6840 at $C400, ...
 * TMS 9918 at $C800, ...
 * 8KByte of ROM at $E000
 ********************************************************************/

 struct hw_device *uart;
 struct hw_device *ptm;
 struct hw_device *rom;
 struct hw_device *ram;
 struct hw_device *tms9918;
 
 void video_init ()
{
    /* 48K RAM from 0000 to BFFF */
    ram = ram_create(VIDEO_RAM_SIZE);
	machine_attach_device(ram);
	machine_map_device (ram , 0, VIDEO_RAM_BASE, VIDEO_RAM_SIZE, MAP_READWRITE );

	/* $C000 and $C001 for 6850 */
	uart = m6850_create(VIDEO_UART_SIZE);
	machine_attach_device(uart);
    machine_map_device (uart, 0, VIDEO_UART_BASE, VIDEO_UART_SIZE, MAP_READWRITE);
	machine_attach_irq (uart);

	/* From $C400 to  $C4xx for 6840 */
	ptm = m6840_create(VIDEO_PTM_SIZE);
	machine_attach_device(ptm);
    machine_map_device (ptm, 0, VIDEO_PTM_BASE, VIDEO_PTM_SIZE, MAP_READWRITE);
	machine_attach_irq (ptm);

	/* From $C800 to  $C8xx for TMS9918 */
	tms9918 = tms9918_create (VIDEO_TMS_SIZE);
	machine_attach_device(tms9918);
	machine_map_device (tms9918, 0, VIDEO_TMS_BASE, VIDEO_TMS_SIZE, MAP_READWRITE);
	machine_attach_irq (tms9918);
	
	/* 8K ROM from E000 to FFFF */
    rom = rom_create (VIDEO_ROM_SIZE);
	machine_attach_device(rom);
    machine_map_device (rom , 0, VIDEO_ROM_BASE, VIDEO_ROM_SIZE, MAP_READWRITE);
}

void video_display()
{
	


	//gui_disassembler((ImVec2){ 640.0f, 360.0f });

	m6850_display(uart, (ImVec2){640.0f, 1.0f});
	tms9918_display (tms9918, (ImVec2){640.0f, 200.0f});


	// Demo 
	//igSetNextWindowPos((ImVec2){460,20}, ImGuiCond_FirstUseEver);
	//igShowDemoWindow(0);
}


 
machine_t video_machine =
{
	.name = "video_test",
	.fault = NULL,
	.init = video_init,
	.dump = NULL,
	.irq = NULL,
	.firq = NULL,
	.display = video_display,
};
