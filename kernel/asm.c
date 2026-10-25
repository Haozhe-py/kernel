#include <stdint.h>
#include "asm.h"

void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

unsigned char inb(unsigned short port)
{
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void invlpg(uint64_t vaddr)
{
    __asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}