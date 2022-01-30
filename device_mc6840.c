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
#include "device_mc6840.h"
#include "device.h"
#include "machine.h"

/*
 * Called by the system to indicate that some number of CPU cycles have passed.
 */
void mc6840_decrement (struct mc6840_port *timer, unsigned int cycles)
{
	/* If CR10 == 1 nothing to do; Counters are stopped */
	if (timer->control_register[0] & (1u << 0))
	{
		printf("Counters are stopped\n");
		return;
	}
	/* Decrement the counters.  Is it zero/negative? */
	timer->count[0] -= cycles;
	timer->count[1] -= cycles;
	timer->count[2] -= cycles;
	
	if (timer->count[1] <= 0)
	{
		/* If interrupt is configured and enabled, generate one now */
		if (timer->control_register[1] & (1u << 6))
		{
			printf("IRQ - Prev:%lu, Now:%lu, Dif:%lu\n",timer->prev_cycles, cycles, cycles-timer->prev_cycles);
			request_irq(timer->int_line);
			timer->status_register |= (1u << 7);
			timer->status_register |= (1u << 1);
			//printf("irq - SR:%02X\n", timer->status_register);
		}
		/* If it is negative, we need to make it positive again.
		If reload is nonzero, add that, to simulate the timer "wrapping".
		Otherwise, fix it at zero. */
		if (timer->count[1] < 0)
		{
			if (timer->reload[1] > 0)
			{
				timer->count[1] += timer->reload[1];
				/* Note: if timer->count is still negative, the reload value
				is lower than the frequency at which the system is updating the
				timers, and we would need to simulate two interrupts here
				perhaps.  For later. */
				if (timer->count[1] < 0)
					sim_error ("timer count = %d, reload = %d\n", timer->count[1], timer->reload[1]);
			}
			else
			{
				timer->count[1] = 0;
			}
		}
	}
}

void mc6840_update (struct hw_device *dev)
{
	struct mc6840_port *timer = (struct mc6840_port *)dev->priv;
	unsigned long cycles = m6809_get_cycles();
	//printf("Update timer - Prev: %lu, Now:%lu, Delta:%lu\n",timer->prev_cycles, cycles, cycles-timer->prev_cycles);
	mc6840_decrement (timer, cycles - timer->prev_cycles);
	timer->prev_cycles = cycles;
}

void mc6840_dump(struct hw_device *dev)
{
	struct mc6840_port *timer = (struct mc6840_port *)dev->priv;
	printf("--- MC6840 dump --- \n");
	printf("SR: %02X\n", timer->status_register);
	printf("CR1: %02X\n", timer->control_register[0]);
	printf("CR2: %02X\n", timer->control_register[1]);
	printf("CR3: %02X\n", timer->control_register[2]);
	printf("Latches 1: %04X\n", timer->reload[0]);
	printf("Latches 2: %04X\n", timer->reload[1]);
	printf("Latches 3: %04X\n", timer->reload[2]);
	printf("Count 1: %04X\n", timer->count[0]);
	printf("Count 2: %04X\n", timer->count[1]);
	printf("Count 3: %04X\n", timer->count[2]);	
}

void mc6840_reset (struct hw_device *dev)
{
	struct mc6840_port *timer = (struct mc6840_port *)dev->priv;
	timer->count[0] = timer->reload[0];
	timer->count[1] = timer->reload[1];
	timer->count[2] = timer->reload[2];	
	timer->status_register = 0x00;
	timer->control_register[0] = 0x00;
	timer->control_register[1] = 0x00;
	timer->control_register[2] = 0x00;
	timer->status_read_since_int[0] = 0;
	timer->status_read_since_int[1] = 0;
	timer->status_read_since_int[2] = 0;
	timer->prev_cycles = m6809_get_cycles ();
}

uint8_t mc6840_read (struct hw_device *dev, unsigned long addr)
{
	struct mc6840_port *timer = (struct mc6840_port *)dev->priv;
	uint8_t c;
	switch (addr)
	{
		case 0:
			//No operation
			printf("No operation\n");
			break;
		case 1:
			//Read status register
			/*
			 ---------------------------------
			 |SR7|SR6|SR5|SR4|SR3|SR2|SR1|SR0|
			 ---------------------------------
			 SR0: Indicateur d'interruption du tempo1
					0: Pas d'interrupt
					1: Inter
			 SR1: Indicateur d'interruption du tempo2
			 SR2: Indicateur d'interruption du tempo3
			 SR7: Indicateur d'interruption commun
					0: Pas d'interruption
					1: Présence d'une interruption sur l'une des 3 tempo
			*/
			if(timer->status_register & (1u << 7))
			{
				/* An interrupt is pending */
				if(timer->status_register & (1u << 0))
				{
					/* Timer 1 */
					timer->status_read_since_int[0] = 1;
				}
				if(timer->status_register & (1u << 1))
				{
					/* Timer 2 */
					timer->status_read_since_int[1] = 1;
				}
				if(timer->status_register & (1u << 2))
				{
					/* Timer 3 */
					timer->status_read_since_int[2] = 1;
				}
			}
			return(timer->status_register);
			break;
		case 2:
			//Read Timer #1 counter MSB
			c = timer->count[0] >> 8;
			return(c);
			break;
		case 3:
			//Read Timer #1 counter LSB
			c = timer->count[0] & 0xFF;
			return (c);
			break;
		case 4:
			//Read Timer #2 counter MSB
			c = timer->count[1] >> 8;
			return(c);
			break;
		case 5:
			//Read Timer #2 counter LSB
			c = timer->count[1] & 0xFF;
			if((timer->status_read_since_int[1] == 1) && (timer->status_register & (1u << 7)) && (timer->status_register & (1u << 1)))
			{
				release_irq(timer->int_line);
				timer->status_read_since_int[1] == 0;
				timer->status_register &= (0u << 1);
				if(!(timer->status_register & (1u << 0) || timer->status_register & (1u << 2)))
				{
					timer->status_register &= (0u << 7);
				}
			}
			return (c);
			break;
		case 6:
			//Read Timer #3 counter MSB
			c = timer->count[2] >> 8;
			return(c);
			break;
		case 7:
			//Read Timer #3 counter LSB
			c = timer->count[2] & 0xFF;
			return (c);
			break;
	}
}

void mc6840_write (struct hw_device *dev, unsigned long addr, uint8_t val)
{
	struct mc6840_port *timer = (struct mc6840_port *)dev->priv;
	uint8_t temp;
	switch (addr)
	{
		case 0:
			//Write Control register CR1 or CR3 depending on CR20
			/* 
			 CR10: Internal Reset Bit
				0: All timers allowed to operate
				1: All timers held in preset state
				
			 CR30: Timer #3 Clock Control
				0: T3 Clock is not prescaled
				1: T3 Clock is prescaled by /8
			*/
			if(timer->control_register[1] & (1u << 0))
			{
				//CR1
				timer->control_register[0] = val;
			}
			else
			{
				//CR3
				timer->control_register[2] = val;
			}
			break;
		case 1:
			//Write Control register CR2
			/*
			 -----------------------------------------
			 |CR27|CR26|CR25|CR24|CR23|CR22|CR21|CR20|
			 -----------------------------------------
			 CR20: Control Register Adress Bit
				0: CR3 may be written
				1: CR1 may be written
			 CR21: Timer #2 Clock Source 
				0: T2 uses external clock source on CX input
				1: T2 uses Enable clock
			 CR22: Timer #2 counting mode control
				0: T2 configured for 16 bit counting mode
				1: T2 configured for dual 8 bit counting mode 
			 CR23: Timer #2 Counter Mode and Interrupt Control
			 CR24: Timer #2 Counter Mode and Interrupt Control
			 CR25: Timer #2 Counter Mode and Interrupt Control
			 CR26: Timer #2 Interrupt Enable
				0: Interrupt Flag masked on IRQ
				1: Interrupt Flag enabled on IRQ
			 CR27: Timer #2 Counter Output Enable
				0: T2 Output masked on output 02
				1: T2 Output enabled on output 02
			 */
			 timer->control_register[1] = val;
			 break;
		case 2:
			//Write Timer #1 counter MSB
			temp = timer->reload[0];
			timer->reload[0] = (val << 8) + temp;
			break;
		case 3:
			//Write Timer #1 counter LSB
			temp = timer->reload[0] >> 8;
			timer->reload[0] = (temp << 8) + val;
			break;			
		case 4:
			//Write Timer #2 counter MSB
			temp = timer->reload[1];
			timer->reload[1] = (val << 8) + temp;
			break;
		case 5:
			//Write Timer #2 counter LSB
			temp = timer->reload[1] >> 8;
			timer->reload[1] = (temp << 8) + val;	
			printf("Latches 2: %04X\n", timer->reload[1]);
			break;
		case 6:
			//Write Timer #3 counter MSB
			temp = timer->reload[2];
			timer->reload[2] = (val << 8) + temp;
			break;
		case 7:
			//Write Timer #3 counter LSB
			temp = timer->reload[2] >> 8;
			timer->reload[2] = (temp << 8) + val;	
			break;
	}
}

struct hw_class mc6840_class =
{
	.name = "mc6840",
	.readonly = 0,
	.reset = mc6840_reset,
	.read = mc6840_read,
	.write = mc6840_write,
	.update = mc6840_update,
	.dump = mc6840_dump,
};

struct hw_device *mc6840_create (unsigned int int_line)
{
	struct mc6840_port *timer = malloc (sizeof (struct mc6840_port));
	timer->reload[0] = 0xFFFF;
	timer->reload[1] = 0xFFFF;
	timer->reload[2] = 0xFFFF;
	timer->count[0] = timer->reload[0];
	timer->count[1] = timer->reload[1];
	timer->count[2] = timer->reload[2];	
	timer->status_register = 0x00;
	timer->control_register[0] = 0x01;
	timer->control_register[1] = 0x00;
	timer->control_register[2] = 0x00;
	timer->prev_cycles = m6809_get_cycles ();
	timer->status_read_since_int[0] = 0;
	timer->status_read_since_int[1] = 0;
	timer->status_read_since_int[2] = 0;
	timer->int_line = int_line;
	return device_attach (&mc6840_class, 128, timer); /* 128 = minimum size*/
}
