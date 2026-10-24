#include "vga_console.h"
#include "asm.h"

curpos_t vga_pos = 0;
volatile char *vga_addr = (volatile char*)0xB8000;

static void update_cursor(curpos_t pos) {
    curpos_t vp = pos;

    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0x0E);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 0x0F);

    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(vp & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)(vp >> 8) & 0xFF);
}

void _vga_putchar(char c)   // Put a character without changing `vga_pos`
{
    vga_addr[vga_pos*2] = c;
    vga_addr[vga_pos*2 +1] = (char)0x07;
}

void vga_putchar(char c)
{
    _vga_putchar(c);
    vga_pos++;
}

void vga_roll(void)
{
    curpos_t src, tgt = 0;
    for(short row = 1; row <= 24; ++row)
    {
        for(short col = 0; col <= 79; ++col)
        {
            src = row * 80 + col;
            tgt = (row-1) * 80 + col;
            vga_addr[tgt*2] = vga_addr[src*2];
            vga_addr[tgt*2+1] = vga_addr[src*2+1];
            vga_addr[src*2] = 0;
            vga_addr[src*2+1] = 0;
        }
    }
    if(vga_pos>=80)
        vga_pos -= 80;
    else
        vga_pos = 0;
}

void vga_print(const char* string)
{
    for(int i=0; string[i]; ++i)
    {
        if(string[i]!='\n')
            vga_putchar(string[i]);
        else
            vga_pos = (vga_pos + 80) / 80 * 80;
        if(vga_pos>=MAX_VGA_POS) 
            vga_roll();
    }
    _vga_putchar('\0');
    update_cursor(vga_pos);
}

void vga_clear(void)
{
    vga_pos = 0;
    for(int i=0; i<MAX_VGA_POS; ++i)
    {
        vga_addr[i*2] = (char)0;
        vga_addr[i*2+1] = (char)0;
    }
    _vga_putchar('\0');
    update_cursor(vga_pos);
} 

void vga_print_hex(uint64_t hex)
{
    char string[16+2+1];
    string[0] = '0'; string[1] = 'x';
    string[16+2] = '\0';
    const char* hexstr = "0123456789ABCDEF";
    for(int i=0; i<16; ++i)
        string[i+2] = hexstr[(hex>>4*(15-i))&0xF];
    vga_print(string);
}

void vga_print_int(uint64_t num)
{
}

void vga_print_double(double num)
{
}