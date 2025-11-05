#ifndef DEVICE_M6850_H
#define DEVICE_M6850_H

struct hw_device* m6850_create (unsigned long size);

/* User functions */

uint8_t m6850_getchar();
void m6850_putchar(uint8_t val);
uint8_t m6850_kbhit();
uint8_t m6850_putready();
void m6850_display(struct hw_device *dev, ImVec2 pos);



#endif
