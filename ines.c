#include "ines.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "mem.h"
#include "ppu.h"
#include "panic.h"



int ines_load(const char *filename, mem_t *mem, ppu_t *ppu)
{
  FILE *fh;
  int c;
  uint8_t header[16];
  uint16_t prg_rom_size;
  uint16_t chr_rom_size;
  uint8_t mapper;
  uint16_t address;
  bool trainer;
  bool mirroring;

  fh = fopen(filename, "rb");
  if (fh == NULL) {
    return -1;
  }

  if (fread(&header, sizeof(uint8_t), 16, fh) != 16) {
    fprintf(stderr, "Unable to read iNES header!\n");
    return -1;
  }

  if (header[0] != 0x4E || header[1] != 0x45 || header[2] != 0x53) {
    fprintf(stderr, "Missing 'NES' ROM signature!\n");
    return -1;
  }

  prg_rom_size  = header[4] * 0x4000;
  chr_rom_size  = header[5] * 0x2000;
  mirroring =  header[6] & 0x1;
  mapper    = (header[6] >> 4) + ((header[7] >> 4) * 0x10);
  trainer   = (header[6] >> 2) & 0x1;

  fprintf(stderr, "PRG ROM Size: 0x%04x\n", prg_rom_size);
  fprintf(stderr, "CHR ROM Size: 0x%04x\n", chr_rom_size);
  fprintf(stderr, "Mirroring: %s\n",
    (mirroring == true) ? "Vertical" : "Horizontal");
  fprintf(stderr, "Mapper: %d\n", mapper);
  fprintf(stderr, "Trainer: %d\n", trainer);

  if (mapper != 0) {
    fprintf(stderr, "Only mapper #0 is currently supported!\n");
    return -1;
  }

  if (trainer == true) {
    fprintf(stderr, "Trainers are not supported!\n");
    return -1;
  }

  /* Set mirroring flag to PPU: */
  ppu->vertical_mirroring = mirroring;

  /* Write from 0x8000 to potentially 0xFFFF if 32K ROM. */
  address = 0x8000;
  while ((c = fgetc(fh)) != EOF) {
    mem_write(mem, address, c);
    address++;
    if (address == 0) {
      break; /* Overflow! */
    }
  }

  /* For 16K ROMs, put a duplicate copy on 0xC000. */
  if (prg_rom_size == 0x4000) {
    address = 0xC000;
    fseek(fh, 16, SEEK_SET);
    while ((c = fgetc(fh)) != EOF) {
      mem_write(mem, address, c);
      address++;
      if (address == 0) {
        break; /* Overflow! */
      }
    }
  }

  if (chr_rom_size == 0x2000) {
    address = 0x0000;
    while ((c = fgetc(fh)) != EOF) {
      ppu->pattern_table[(address >> 12) & 0x1][address & 0xFFF] = c;
      address++;
    }
  }

  fclose(fh);
  return 0;
}



