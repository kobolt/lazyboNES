#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "ppu.h"
#include "mem.h"
#include "gui.h"
#include "cli.h"
#include "panic.h"



static uint8_t ppu_mem_read(ppu_t *ppu, uint16_t address)
{
  uint8_t value = ppu->vram_buffer;

  if (address <= 0x0FFF) {
    ppu->vram_buffer = ppu->pattern_table[0][address];
    return value;

  } else if (address <= 0x1FFF) {
    ppu->vram_buffer = ppu->pattern_table[1][address - 0x1000];
    return value;

  } else if (address <= 0x2FFF) {
    ppu->vram_buffer = ppu->name_table[address - 0x2000];
    return value;

  } else if (address <= 0x3EFF) {
    ppu->vram_buffer = ppu->name_table[address - 0x3000]; /* Mirror */
    return value;

  } else if (address <= 0x3FFF) {
    return ppu->palette_ram[(address - 0x3F00) % 0x20];

  } else {
    panic("PPU memory read outside bounds: 0x%04x\n", address);
    return 0;
  }
}



static void ppu_mem_write(ppu_t *ppu, uint16_t address, uint8_t value)
{
  if (address <= 0x0FFF) {
    ppu->pattern_table[0][address] = value;

  } else if (address <= 0x1FFF) {
    ppu->pattern_table[1][address - 0x1000] = value;

  } else if (address <= 0x23FF) {
    ppu->name_table[address - 0x2000] = value;
    if (ppu->vertical_mirroring) {
      ppu->name_table[(0x800 + address) - 0x2000] = value;
    } else {
      ppu->name_table[(0x400 + address) - 0x2000] = value;
    }

  } else if (address <= 0x27FF) {
    ppu->name_table[(0x400 + address) - 0x2400] = value;
    if (ppu->vertical_mirroring) {
      ppu->name_table[(0xC00 + address) - 0x2400] = value;
    } else {
      ppu->name_table[address - 0x2400] = value;
    }

  } else if (address <= 0x2BFF) {
    ppu->name_table[(0x800 + address) - 0x2800] = value;
    if (ppu->vertical_mirroring) {
      ppu->name_table[address - 0x2800] = value;
    } else {
      ppu->name_table[(0xC00 + address) - 0x2800] = value;
    }

  } else if (address <= 0x2FFF) {
    ppu->name_table[(0xC00 + address) - 0x2C00] = value;
    if (ppu->vertical_mirroring) {
      ppu->name_table[(0x400 + address) - 0x2C00] = value;
    } else {
      ppu->name_table[(0x800 + address) - 0x2C00] = value;
    }

  } else if (address <= 0x33FF) {
    ppu->name_table[address - 0x3000] = value; /* Mirror */
    if (ppu->vertical_mirroring) {
      ppu->name_table[(0x800 + address) - 0x3000] = value;
    } else {
      ppu->name_table[(0x400 + address) - 0x3000] = value;
    }

  } else if (address <= 0x37FF) {
    ppu->name_table[(0x400 + address) - 0x3400] = value; /* Mirror */
    if (ppu->vertical_mirroring) {
      ppu->name_table[(0xC00 + address) - 0x3400] = value;
    } else {
      ppu->name_table[address - 0x3400] = value;
    }

  } else if (address <= 0x3BFF) {
    ppu->name_table[(0x800 + address) - 0x3800] = value; /* Mirror */
    if (ppu->vertical_mirroring) {
      ppu->name_table[address - 0x3800] = value;
    } else {
      ppu->name_table[(0xC00 + address) - 0x3800] = value;
    }

  } else if (address <= 0x3EFF) {
    ppu->name_table[(0xC00 + address) - 0x3C00] = value; /* Mirror */
    if (ppu->vertical_mirroring) {
      ppu->name_table[(0x400 + address) - 0x3C00] = value;
    } else {
      ppu->name_table[(0x800 + address) - 0x3C00] = value;
    }

  } else if (address <= 0x3FFF) {
    value &= ~0b11000000; /* Filter out the two upper bytes. */
    ppu->palette_ram[(address - 0x3F00) % 0x20] = value;

    /* Palette Mirroring */
    if (address % 4 == 0) {
      if ((address >> 4) % 2 == 0) {
        ppu->palette_ram[(address - 0x3EF0) % 0x20] = value;
      } else {
        ppu->palette_ram[(address - 0x3F10) % 0x20] = value;
      }
    }

  } else {
    panic("PPU memory write (%02x) outside bounds: 0x%04x\n", value, address);
  }
}



static uint8_t ppu_read_hook(void *ppu, uint16_t address)
{
  switch (address) {
  case PPU_CTRL:
    return ((ppu_t *)ppu)->ctrl;

  case PPU_MASK:
    return ((ppu_t *)ppu)->mask;

  case PPU_STATUS:
    ((ppu_t *)ppu)->status_was_accessed = true;
    return ((ppu_t *)ppu)->status;

  case PPU_OAM_ADDR:
    return ((ppu_t *)ppu)->oam_addr;

  case PPU_SCROLL:
    return 0;

  case PPU_ADDR:
    return 0;

  case PPU_DATA:
    ((ppu_t *)ppu)->data_was_accessed = true;
    return ppu_mem_read((ppu_t *)ppu, ((ppu_t *)ppu)->addr);

  default:
    panic("PPU read on unhandled address: 0x%04x\n", address);
    return 0;
  }
}



static void ppu_write_hook(void *ppu, uint16_t address, uint8_t value)
{
  switch (address) {
  case PPU_CTRL:
    ((ppu_t *)ppu)->ctrl = value;
    ((ppu_t *)ppu)->status_leftover = value & 0b00011111;
    break;

  case PPU_MASK:
    ((ppu_t *)ppu)->mask = value;
    ((ppu_t *)ppu)->status_leftover = value & 0b00011111;
    break;

  case PPU_OAM_ADDR:
    ((ppu_t *)ppu)->oam_addr = value;
    ((ppu_t *)ppu)->status_leftover = value & 0b00011111;
    break;

  case PPU_SCROLL:
    if (((ppu_t *)ppu)->addr_latch == true) {
      ((ppu_t *)ppu)->scroll_y = value;
      ((ppu_t *)ppu)->addr_latch = false;
    } else {
      ((ppu_t *)ppu)->scroll_x = value;
      ((ppu_t *)ppu)->addr_latch = true;
    }
    ((ppu_t *)ppu)->status_leftover = value & 0b00011111;
    break;

  case PPU_ADDR:
    if (((ppu_t *)ppu)->addr_latch == true) {
      ((ppu_t *)ppu)->addr += value;
      ((ppu_t *)ppu)->addr_latch = false;
    } else {
      ((ppu_t *)ppu)->addr = (value * 256);
      ((ppu_t *)ppu)->addr_latch = true;
    }
    if (((ppu_t *)ppu)->addr > 0x3FFF) {
      panic("Illegal PPU address latched: 0x%04x\n", ((ppu_t *)ppu)->addr);
    }
    ((ppu_t *)ppu)->status_leftover = value & 0b00011111;
    break;

  case PPU_DATA:
    ((ppu_t *)ppu)->data_was_accessed = true;
    ppu_mem_write((ppu_t *)ppu, ((ppu_t *)ppu)->addr, value);
    ((ppu_t *)ppu)->status_leftover = value & 0b00011111;
    break;

  default:
    panic("PPU write (0x%02x) on unhandled address: 0x%04x\n", value, address);
    break;
  }
}



void ppu_init(ppu_t *ppu, mem_t *mem)
{
  int i, j;

  /* Memory connections: */
  mem->ppu = ppu;
  mem->ppu_read  = ppu_read_hook;
  mem->ppu_write = ppu_write_hook;

  /* Registers: */
  ppu->ctrl     = 0;
  ppu->mask     = 0;
  ppu->status   = 0;
  ppu->oam_addr = 0;
  ppu->oam_data = 0;
  ppu->scroll_x = 0;
  ppu->scroll_y = 0;
  ppu->addr     = 0;
  ppu->data     = 0;

  /* Timing: */
  ppu->frame_no = 0;
  ppu->scanline = -1;
  ppu->dot      = 0;

  /* Internal flags: */
  ppu->status_was_accessed = false;
  ppu->data_was_accessed   = false;
  ppu->addr_latch  = false;
  ppu->trigger_nmi = false;
  ppu->vram_buffer = 0;
  ppu->vertical_mirroring = false;

  /* PPU memory: */
  for (i = 0; i < PPU_PATTERN_TABLES; i++) {
    for (j = 0; j < PPU_SIZE_PATTERN_TABLE; j++) {
      ppu->pattern_table[i][j] = 0xee + i; /* Value for easier debugging. */
    }
  }
  for (i = 0; i < PPU_NAME_TABLES * PPU_SIZE_NAME_TABLE; i++) {
    ppu->name_table[i] = 0xdd; /* Value for easier debugging. */
  }
  for (i = 0; i < PPU_SIZE_PALETTE_RAM; i++) {
    ppu->palette_ram[i] = 0x0;
  }
  for (i = 0; i < PPU_SIZE_SPRITE_RAM; i++) {
    ppu->sprite_ram[i] = 0x0;
  }
}



static void ppu_draw_background(ppu_t *ppu, uint8_t pixels[])
{
  uint8_t nt, at;
  uint8_t base_htile, htile, vtile;
  uint8_t plane1, plane2;
  uint8_t color, palette_index, palette_group;
  uint8_t pixel_no;
  uint8_t x_pixel;
  uint16_t nt_offset;

  vtile = (ppu->scanline / 8);

  for (base_htile = 0; base_htile < 32; base_htile++) {
    htile = base_htile + (ppu->scroll_x / 8);
    if (htile >= 32) {
      if (ppu->nametable_sel == 0) {
        nt_offset = PPU_SIZE_NAME_TABLE; /* Use nametable 1 instead. */
      } else {
        nt_offset = 0; /* Use nametable 0 instead. */
      }
    } else {
      nt_offset = ppu->nametable_sel * PPU_SIZE_NAME_TABLE;
    }
    htile %= 32;

    nt = ppu->name_table[nt_offset + htile + (vtile * 32)];
    at = ppu->name_table[nt_offset + 0x3C0 + (htile / 4) + ((vtile / 4) * 8)];

    if (ppu->scanline % 8 == 0) {
      cli_draw_tile(vtile, base_htile, ppu->bg_tile_sel, nt);
    }

    if (((htile % 4) <= 1) && ((vtile % 4) <= 1)) {
      palette_group = at & 0x3;
    } else if (((htile % 4) >= 2) && ((vtile % 4) <= 1)) {
      palette_group = (at >> 2) & 0x3;
    } else if (((htile % 4) <= 1) && ((vtile % 4) >= 2)) {
      palette_group = (at >> 4) & 0x3;
    } else {
      palette_group = (at >> 6) & 0x3;
    }

    plane1 = ppu->pattern_table[ppu->bg_tile_sel]
      [(nt << 4) + (ppu->scanline % 8)];
    plane2 = ppu->pattern_table[ppu->bg_tile_sel]
      [(nt << 4) + (ppu->scanline % 8) + 8];

    for (pixel_no = 0; pixel_no < 8; pixel_no++) {
      palette_index = ((plane1 >> pixel_no) & 1) +
                     (((plane2 >> pixel_no) & 1) * 2);
      if (palette_index == 0) {
        color = ppu->palette_ram[palette_index]; /* Always read 0x3F00. */
      } else {
        color = ppu->palette_ram[(palette_group * 4) + palette_index];
      }
      x_pixel = ((base_htile * 8) + (7 - pixel_no)) - (ppu->scroll_x % 8);
      pixels[x_pixel % 256] = color;
    }
  }
}



static void ppu_draw_sprites(ppu_t *ppu, uint8_t pixels[])
{
  uint8_t nt;
  uint8_t plane1, plane2;
  uint8_t color, palette_index, palette_group;
  uint8_t pixel_no;
  uint8_t y_offset;
  uint16_t x_pixel;
  int sprite;

  for (sprite = 0; sprite < PPU_SIZE_SPRITE_RAM; sprite += 4) {
    if (ppu->scanline >= ppu->sprite_ram[sprite] + 1 &&
        ppu->scanline <= ppu->sprite_ram[sprite] + 8) {

      nt = ppu->sprite_ram[sprite+1];

      if (ppu->scanline % 8 == 0) {
        cli_draw_tile(ppu->sprite_ram[sprite] / 8,
                     (ppu->sprite_ram[sprite+3] + 4) / 8,
                      ppu->sprite_tile_sel, nt);
      }

      if ((ppu->sprite_ram[sprite+2] >> 7) & 0x1) { /* Flip vertically. */
        y_offset = 7 - (ppu->scanline - ppu->sprite_ram[sprite] - 1);
      } else { /* Do not flip vertically. */
        y_offset = ppu->scanline - ppu->sprite_ram[sprite] - 1;
      }
      palette_group = (ppu->sprite_ram[sprite+2] & 0x3) + 4;

      plane1 = ppu->pattern_table[ppu->sprite_tile_sel]
        [(nt << 4) + (y_offset % 8)];
      plane2 = ppu->pattern_table[ppu->sprite_tile_sel]
        [(nt << 4) + (y_offset % 8) + 8];

      for (pixel_no = 0; pixel_no < 8; pixel_no++) {
        palette_index = ((plane1 >> pixel_no) & 1) +
                       (((plane2 >> pixel_no) & 1) * 2);

        if ((ppu->sprite_ram[sprite+2] >> 6) & 0x1) { /* Flip horizontally. */
          x_pixel = ppu->sprite_ram[sprite+3] + pixel_no;
        } else { /* Do not flip horizontally. */
          x_pixel = ppu->sprite_ram[sprite+3] + (7 - pixel_no);
        }
        if (x_pixel > 0xFF) {
          continue; /* Out of bounds, do not render. */
        }

        if (palette_index != 0) { /* Not transparent. */
          if (sprite == 0 &&
            pixels[x_pixel] > 0) {
            ppu->sprite_0_hit = 1;
          }
          color = ppu->palette_ram[(palette_group * 4) + palette_index];
          pixels[x_pixel] = color;
        }
      }
    }
  }
}



void ppu_execute(ppu_t *ppu)
{
  static uint8_t pixels[256]; /* On 1 scanline. */

  if (ppu->status_was_accessed) {
    ppu->status_was_accessed = false;
    ppu->vblank = 0;
    ppu->addr_latch = false;
  }

  if (ppu->data_was_accessed) {
    ppu->data_was_accessed = false;
    if (ppu->vram_increment == 1) {
      ppu->addr += 32;
    } else {
      ppu->addr += 1;
    }
    ppu->addr &= 0x7FFF; /* VRAM address register can only be 15 bits. */
  }

  if (ppu->scanline == -1 && ppu->dot == 1) {
    ppu->vblank = 0;
    ppu->sprite_0_hit = 0;
    ppu->nametable_sel = 0;

  } else if (ppu->scanline >= 0 && ppu->scanline <= 239 && ppu->dot == 0) {
    ppu_draw_background(ppu, pixels);
    ppu_draw_sprites(ppu, pixels);
    gui_draw_scanline(ppu->scanline, pixels);

  } else if (ppu->scanline == 241 && ppu->dot == 1) {
    ppu->vblank = 1;

    if (ppu->nmi_enable) {
      ppu->trigger_nmi = true;
    }
  }

  ppu->dot++;
  if (ppu->dot >= 341) {
    ppu->dot = 0;
    ppu->scanline++;
    if (ppu->scanline >= 262) { /* NTSC */
      ppu->scanline = -1;
      ppu->frame_no++;
    }
  }
}



void ppu_dump(FILE *fh, ppu_t *ppu)
{
  fprintf(fh, "Frame Number: %u\n", ppu->frame_no);
  fprintf(fh, "Scanline/Dot: %d/%d\n", ppu->scanline, ppu->dot);
  fprintf(fh, "Control   : 0x%02x\n", ppu->ctrl);
  fprintf(fh, "Mask      : 0x%02x\n", ppu->mask);
  fprintf(fh, "Status    : 0x%02x\n", ppu->status);
  fprintf(fh, "Address   : 0x%04x\n", ppu->addr);
  fprintf(fh, "Scroll X/Y: %d/%d\n", ppu->scroll_x, ppu->scroll_y);
}



void ppu_pattern_table_dump(FILE *fh, ppu_t *ppu, int table_no, int pattern_no)
{
  int hpixel, vpixel, pixel;
  uint8_t plane1, plane2;

  if (table_no < 0 || table_no >= PPU_PATTERN_TABLES) {
    fprintf(fh, "Invalid pattern table number: %d\n", table_no);
    return;
  }

  if (pattern_no < 0 || pattern_no >= 256) {
    fprintf(fh, "Invalid pattern number: %d\n", table_no);
    return;
  }

  for (vpixel = 0; vpixel < 8; vpixel++) {
    plane1 = ppu->pattern_table[table_no][(pattern_no * 16) + vpixel];
    plane2 = ppu->pattern_table[table_no][(pattern_no * 16) + vpixel + 8];
    for (hpixel = 7; hpixel > 0; hpixel--) {
      pixel = ((plane1 >> hpixel) & 1) + (((plane2 >> hpixel) & 1) * 2);
      if (pixel == 0) fprintf(fh, " ");
      if (pixel == 1) fprintf(fh, "=");
      if (pixel == 2) fprintf(fh, ":");
      if (pixel == 3) fprintf(fh, "#");
    }
    fprintf(fh, "\n");
  }
}



void ppu_name_table_dump(FILE *fh, ppu_t *ppu, int table_no)
{
  int htile, vtile;
  uint8_t nt;

  if (table_no < 0 || table_no >= PPU_NAME_TABLES) {
    fprintf(fh, "Invalid name table number: %d\n", table_no);
    return;
  }
  table_no *= PPU_SIZE_NAME_TABLE;

  for (vtile = 0; vtile < 30; vtile++) {
    fprintf(fh, "$%04x  ", 0x2000 + (table_no * 0x400) + (vtile * 32));
    for (htile = 0; htile < 32; htile++) {
      nt = ppu->name_table[table_no + htile + (vtile * 32)];
      fprintf(fh, "%02x", nt);
    }
    fprintf(fh, "\n");
  }
}



void ppu_attribute_table_dump(FILE *fh, ppu_t *ppu, int table_no)
{
  int htile, vtile;
  uint8_t at;

  if (table_no < 0 || table_no >= PPU_NAME_TABLES) {
    fprintf(fh, "Invalid attribute table number: %d\n", table_no);
    return;
  }
  table_no *= PPU_SIZE_NAME_TABLE;

  fprintf(fh, " | 0| 1| 2| 3| 4| 5| 6| 7|\n");
  fprintf(fh, " |--+--+--+--+--+--+--+--|\n");
  for (vtile = 0; vtile < 30; vtile += 4) {
    fprintf(fh, "%d|", vtile / 4);
    for (htile = 0; htile < 32; htile += 4) {
      at = ppu->name_table[table_no + 0x3C0 + (htile / 4) + ((vtile / 4) * 8)];
      fprintf(fh, "%02x|", at);
    }
    fprintf(fh, "\n |--+--+--+--+--+--+--+--|\n");
  }
}



void ppu_palette_ram_dump(FILE *fh, ppu_t *ppu)
{
  int i;

  for (i = 0; i < PPU_SIZE_PALETTE_RAM; i++) {
    fprintf(fh, "%02x ", ppu->palette_ram[i]);
    if (i % 16 == 15) {
      fprintf(fh, "\n");
    } else if (i % 4 == 3) {
      fprintf(fh, "| ");
    }
  }
}



void ppu_sprite_ram_dump(FILE *fh, ppu_t *ppu)
{
  int i;

  for (i = 0; i < PPU_SIZE_SPRITE_RAM; i += 4) {
    fprintf(fh, "%02d: X=%02x Y=%02x Tile=%02x Attrib=%02x\n", i / 4,
      ppu->sprite_ram[i+3], ppu->sprite_ram[i],
      ppu->sprite_ram[i+1], ppu->sprite_ram[i+2]);
  }
}



