#ifndef _PPU_H
#define _PPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "mem.h"

#define PPU_PATTERN_TABLES 2
#define PPU_NAME_TABLES 4

#define PPU_SIZE_PATTERN_TABLE 0x1000
#define PPU_SIZE_NAME_TABLE    0x400
#define PPU_SIZE_PALETTE_RAM   0x20
#define PPU_SIZE_SPRITE_RAM    0x100

typedef struct ppu_s {
  union {
    struct {
      uint8_t nametable_sel   : 2;
      uint8_t vram_increment  : 1;
      uint8_t sprite_tile_sel : 1;
      uint8_t bg_tile_sel     : 1;
      uint8_t sprite_height   : 1;
      uint8_t master_slave    : 1;
      uint8_t nmi_enable      : 1;
    };
    uint8_t ctrl;
  };
  union {
    struct {
      uint8_t greyscale        : 1;
      uint8_t bg_lc_enable     : 1;
      uint8_t sprite_lc_enable : 1;
      uint8_t bg_enable        : 1;
      uint8_t sprite_enable    : 1;
      uint8_t emphasis_red     : 1;
      uint8_t emphasis_green   : 1;
      uint8_t emphasis_blue    : 1;
    };
    uint8_t mask;
  };
  union {
    struct {
      uint8_t status_leftover : 5;
      uint8_t sprite_overflow : 1;
      uint8_t sprite_0_hit    : 1;
      uint8_t vblank          : 1;
    };
    uint8_t status;
  };
  uint8_t oam_addr;
  uint8_t oam_data;
  uint8_t scroll_x;
  uint8_t scroll_y;
  uint16_t addr;
  uint8_t data;

  uint32_t frame_no;
  int16_t scanline;
  int16_t dot;

  bool status_was_accessed;
  bool data_was_accessed;
  bool addr_latch;
  bool trigger_nmi;
  uint8_t vram_buffer;
  bool vertical_mirroring;

  uint8_t pattern_table[PPU_PATTERN_TABLES][PPU_SIZE_PATTERN_TABLE];
  uint8_t name_table[PPU_NAME_TABLES * PPU_SIZE_NAME_TABLE];
  uint8_t palette_ram[PPU_SIZE_PALETTE_RAM];
  uint8_t sprite_ram[PPU_SIZE_SPRITE_RAM];
} ppu_t;

#define PPU_CTRL     0x2000
#define PPU_MASK     0x2001
#define PPU_STATUS   0x2002
#define PPU_OAM_ADDR 0x2003
#define PPU_OAM_DATA 0x2004
#define PPU_SCROLL   0x2005
#define PPU_ADDR     0x2006
#define PPU_DATA     0x2007

void ppu_init(ppu_t *ppu, mem_t *mem);
void ppu_execute(ppu_t *ppu);
void ppu_dump(FILE *fh, ppu_t *ppu);
void ppu_pattern_table_dump(FILE *fh, ppu_t *ppu, int table_no, int pattern_no);
void ppu_name_table_dump(FILE *fh, ppu_t *ppu, int table_no);
void ppu_attribute_table_dump(FILE *fh, ppu_t *ppu, int table_no);
void ppu_palette_ram_dump(FILE *fh, ppu_t *ppu);
void ppu_sprite_ram_dump(FILE *fh, ppu_t *ppu);

#endif /* _PPU_H */
