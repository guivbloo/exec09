#include <stdio.h>
#include <stdint.h>
#include "device_m6850.h"

FILE *in_file;
uint16_t size;
uint8_t checksum = 0;
uint16_t state = 0;
uint16_t cpt = 0;


void console_putchar(uint8_t val)
{
    m6850_putchar(val);
	printf("(csl) -> 0x%02X\n", val);
}

uint8_t console_getchar()
{
    uint8_t val = m6850_getchar();
	printf("(csl) <- 0x%02X", val);
    if((val>0x1F) && (val<0x7F))
		printf(" (%c)",val);
	printf("\n");
    return val;
}

void console_run()
{
	uint8_t val;
    switch (state)
    {
        case 0:
        	if(m6850_kbhit())
	        {
		        val = console_getchar();
                if(val == 0x23) /* '#' */
                {
                    if(m6850_putready() == 0)
                    {
                        val = 0xAA;
                        console_putchar(val);
                    }
                }
                else if(val == 0x11) /* Get CSW */
                {
                    if(m6850_putready() == 0 && in_file != NULL)
                    {
                        val = 0x11;
                        console_putchar(val);
                        state++;
                        printf("(csl) Request CSW received\n");
                    }
                }
            }
            break;
        case 1:
            if(m6850_putready() == 0)
            {
                val = size >> 8;
                console_putchar(val);
                state++;
            }
            break;
        case 2:
            if(m6850_putready() == 0)
            {
                val = size;
                console_putchar(val);
                state++;
            }
            break;
        case 3:
            if(m6850_putready() == 0)
            {
                val = checksum;
                console_putchar(val);
                state++;
            }
            break;           
        case 4:
            if(cpt < size)
            {
                if(m6850_putready() == 0)
                {
                    val = fgetc(in_file);
                    console_putchar(val);
                    cpt++;
                }
            }
            else
            {
                printf("(csl) CSW sent\n");
                cpt = 0;
                state = 0;
            }
            break;

	}

}

void console_init()
{
    int i;
    in_file  = fopen("test/csw.bin", "r"); // read only 
    fseek(in_file, 0, SEEK_END); // seek to end of file
    size = ftell(in_file); // get current file pointer
    fseek(in_file, 0, SEEK_SET); // seek back to beginning of file
    for(i=0;i<size;i++)
    {
        checksum += fgetc(in_file);
    }
    fseek(in_file, 0, SEEK_SET); // seek back to beginning of file
    printf("(csl) CSW loaded: %d bytes. CS: %02X\n", size, checksum);
}


