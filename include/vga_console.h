/* VGA Console ( 80 Cols * 25 Rows) */

#include <stdint.h>

typedef uint16_t curpos_t;

#define MAX_VGA_POS ((curpos_t)25*80)

void vga_putchar(char);
void vga_roll(void);
void vga_print(const char*);
void vga_clear(void);
