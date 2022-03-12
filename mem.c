#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "mem.h"
#include "ppu.h"
#include "apu.h"
#include "panic.h"



void mem_init(mem_t *mem)
{
  int i;

  /* Initialize all to 0xFF since 0x00 is the opcode for BRK. */
  for (i = 0; i < MEM_SIZE_RAM; i++) {
    mem->ram[i] = 0xff;
  }
  for (i = 0; i < MEM_SIZE_CART; i++) {
    mem->cart[i] = 0xff;
  }

  mem->ppu_read  = NULL;
  mem->ppu_write = NULL;
  mem->apu_read  = NULL;
  mem->apu_write = NULL;
  mem->ppu = NULL;
  mem->apu = NULL;
}



uint8_t mem_read(mem_t *mem, uint16_t address)
{
  if (address <= 0x07FF) {
    return mem->ram[address];

  } else if (address <= 0x0FFF) {
    return mem->ram[address - 0x800]; /* Mirror */

  } else if (address <= 0x17FF) {
    return mem->ram[address - 0x1000]; /* Mirror */

  } else if (address <= 0x1FFF) {
    return mem->ram[address - 0x1800]; /* Mirror */

  } else if (address <= 0x3FFF) {
    if (mem->ppu_read != NULL && mem->ppu != NULL) {
      return (mem->ppu_read)(mem->ppu, (address % 0x8) + 0x2000);
    } else {
      panic("PPU read hook not installed! Address: 0x%04x\n", address);
    }

  } else if (address <= 0x401F) {
    if (mem->apu_read != NULL && mem->apu != NULL) {
      return (mem->apu_read)(mem->apu, address);
    } else {
      panic("APU read hook not installed! Address: 0x%04x\n", address);
    }

  }

  return mem->cart[address - 0x4020];
}



void mem_write(mem_t *mem, uint16_t address, uint8_t value)
{
  if (address <= 0x07FF) {
    mem->ram[address] = value;

  } else if (address <= 0x0FFF) {
    mem->ram[address - 0x800] = value; /* Mirror */

  } else if (address <= 0x17FF) {
    mem->ram[address - 0x1000] = value; /* Mirror */

  } else if (address <= 0x1FFF) {
    mem->ram[address - 0x1800] = value; /* Mirror */

  } else if (address <= 0x3FFF) {
    if (mem->ppu_write != NULL && mem->ppu != NULL) {
      (mem->ppu_write)(mem->ppu, (address % 0x8) + 0x2000, value);
    } else {
      panic("PPU write hook not installed! Address: 0x%04x\n", address);
    }

  } else if (address == APU_OAM_DMA) {
    /* Special sprite data DMA transfer. */
    if (mem->ppu != NULL) {
      for (int i = 0; i < PPU_SIZE_SPRITE_RAM; i++) {
        ((ppu_t *)mem->ppu)->sprite_ram[i] = mem_read(mem, (value * 256) + i);
      }
    } else {
      panic("PPU reference not installed!\n");
    }

  } else if (address <= 0x401F) {
    if (mem->apu_write != NULL && mem->apu != NULL) {
      (mem->apu_write)(mem->apu, address, value);
    } else {
      panic("APU write hook not installed! Address: 0x%04x\n", address);
    }
  
  } else {
    mem->cart[address - 0x4020] = value;
  }
}



void mem_dump(FILE *fh, mem_t *mem, uint16_t start, uint16_t end)
{
  int i;

  if (start <= 0x07FF && end <= 0x07FF) {
    for (i = start; i <= end; i++) {
      if (i % 16 == 0) {
        fprintf(fh, "$%04x   ", i);
      }
      fprintf(fh, "%02x ", mem->ram[i]);
      if (i % 16 == 15) {
        fprintf(fh, "\n");
      }
    }
  } else if (start >= 0x4020 && end >= 0x4020) {
    for (i = start; i <= end; i++) {
      if (i % 16 == 0) {
        fprintf(fh, "$%04x   ", i);
      }
      fprintf(fh, "%02x ", mem->cart[i - 0x4020]);
      if (i % 16 == 15) {
        fprintf(fh, "\n");
      }
    }
  } else {
    fprintf(fh, "???\n");
  }
}



