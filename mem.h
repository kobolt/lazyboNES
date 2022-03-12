#ifndef _MEM_H
#define _MEM_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t (*mem_read_hook_t)(void *, uint16_t);
typedef void (*mem_write_hook_t)(void *, uint16_t, uint8_t);

#define MEM_SIZE_RAM  0x800
#define MEM_SIZE_CART 0xBFE0

typedef struct mem_s {
  uint8_t ram[MEM_SIZE_RAM];   /* 0x0000 -> 0x07FF */
  uint8_t cart[MEM_SIZE_CART]; /* 0x4020 -> 0xFFFF */
  mem_read_hook_t  ppu_read;
  mem_write_hook_t ppu_write;
  mem_read_hook_t  apu_read;
  mem_write_hook_t apu_write;
  void *ppu;
  void *apu;
} mem_t;

#define MEM_PAGE_STACK 0x100

#define MEM_VECTOR_NMI_LOW    0xFFFA
#define MEM_VECTOR_NMI_HIGH   0xFFFB
#define MEM_VECTOR_RESET_LOW  0xFFFC
#define MEM_VECTOR_RESET_HIGH 0xFFFD
#define MEM_VECTOR_IRQ_LOW    0xFFFE
#define MEM_VECTOR_IRQ_HIGH   0xFFFF

void mem_init(mem_t *mem);
uint8_t mem_read(mem_t *mem, uint16_t address);
void mem_write(mem_t *mem, uint16_t address, uint8_t value);
void mem_dump(FILE *fh, mem_t *mem, uint16_t start, uint16_t end);

#endif /* _MEM_H */
