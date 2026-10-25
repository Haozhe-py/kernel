#include <stdint.h>

void outb(unsigned short port, unsigned char value);
unsigned char inb(unsigned short port);
void invlpg(uint64_t);