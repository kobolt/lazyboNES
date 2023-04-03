#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "fds.h"
#include "mem.h"
#include "ppu.h"
#include "panic.h"

#define FDS_WAIT_CYCLES 150



static uint8_t fds_read_hook(void *fds, uint16_t address)
{
  uint8_t value;

  switch (address) {
  case FDS_DISK_STATUS:
    value  = 0x80; /* CRC passed, R/W enabled. */
    value |=  ((fds_t *)fds)->timer_irq_occurred > 0 ? 1 : 0;
    value |= (((fds_t *)fds)->byte_transferred   > 0 ? 1 : 0) << 1;
    value |= (((fds_t *)fds)->image_index == 0 ? 1 : 0) << 6;
    ((fds_t *)fds)->ack_disk_irq  = true;
    ((fds_t *)fds)->ack_timer_irq = true;
    ((fds_t *)fds)->byte_transferred = false;
    return value;

  case FDS_DATA_READ:
    ((fds_t *)fds)->ack_disk_irq = true;
    ((fds_t *)fds)->byte_transferred = false;
    return ((fds_t *)fds)->data_read;

  case FDS_DRIVE_STATUS:
    value  =  ((fds_t *)fds)->disk_inserted      > 0 ? 0 : 1;
    value |= (((fds_t *)fds)->disk_ready         > 0 ? 0 : 1) << 1;
    value |= (((fds_t *)fds)->disk_write_protect > 0 ? 1 : 0) << 2;
    return value;

  case FDS_EXT_CONNECTOR_READ:
    return 0x80; /* Battery Good */

  default:
    if (address >= 0x800 && address <= 0x1FFF) {
      return ((fds_t *)fds)->ram[address - 0x800];
    } else {
      return 0;
    }
  }
}



static void fds_write_hook(void *fds, uint16_t address, uint8_t value)
{
  switch (address) {
  case FDS_TIMER_IRQ_RELOAD_LO:
    ((fds_t *)fds)->timer_reload =
      (((fds_t *)fds)->timer_reload & 0xFF00) | value;
    break;

  case FDS_TIMER_IRQ_RELOAD_HI:
    ((fds_t *)fds)->timer_reload =
      (((fds_t *)fds)->timer_reload & 0x00FF) | (value << 8);
    break;

  case FDS_TIMER_IRQ_CONTROL:
    ((fds_t *)fds)->timer_irq_repeat =  value       & 0x1;
    ((fds_t *)fds)->timer_irq_enable = (value >> 1) & 0x1;
    if (((fds_t *)fds)->timer_irq_enable) {
      ((fds_t *)fds)->timer_count = ((fds_t *)fds)->timer_reload;
    } else {
      ((fds_t *)fds)->ack_timer_irq = true;
    }
    break;

  case FDS_MASTER_IO_ENABLE:
    ((fds_t *)fds)->io_enable_disk  =  value       & 0x1;
    ((fds_t *)fds)->io_enable_sound = (value >> 1) & 0x1;
    break;

  case FDS_DATA_WRITE:
    ((fds_t *)fds)->ack_disk_irq = true;
    ((fds_t *)fds)->byte_transferred = false;
    /* Ignored */
    break;

  case FDS_CONTROL:
    ((fds_t *)fds)->ctrl = value;
    ((fds_t *)fds)->ppu->vertical_mirroring = !((fds_t *)fds)->mirroring;
    ((fds_t *)fds)->ack_disk_irq = true;
    break;

  case FDS_EXT_CONNECTOR_WRITE:
    /* Ignored */
    break;

  default:
    if (address >= 0x800 && address <= 0x1FFF) {
      ((fds_t *)fds)->ram[address - 0x800] = value;
    }
    break;
  }
}



int fds_bios_load(const char *filename, mem_t *mem)
{
  FILE *fh;
  int c;
  uint16_t address;

  fh = fopen(filename, "rb");
  if (fh == NULL) {
    return -1;
  }

  address = 0xE000;
  while ((c = fgetc(fh)) != EOF) {
    mem_write(mem, address, c);
    address++;
    if (address == 0) {
      break; /* Overflow! */
    }
  }

  fclose(fh);
  return 0;
}



int fds_image_load(const char *filename, fds_t *fds)
{
  FILE *fh;
  int c;
  int n;

  fh = fopen(filename, "rb");
  if (fh == NULL) {
    return -1;
  }

  n = 0;
  while ((c = fgetc(fh)) != EOF) {
    /* Skip possible 16 byte 'FDS' header if it exists: */
    if (n == 0 && c == 'F') {
      fseek(fh, 16, SEEK_SET);
      continue;
    }
    fds->image[n] = c;
    n++;
    if (n >= FDS_IMAGE_SIZE) {
      break;
    }
  }

  /* Mark everything ready and write protected: */
  fds->disk_inserted      = true;
  fds->disk_ready         = true;
  fds->disk_write_protect = true;

  fclose(fh);
  return 0;
}



void fds_init(fds_t *fds, mem_t *mem, ppu_t *ppu)
{
  int i;

  /* Memory connections: */
  mem->fds = fds;
  mem->fds_read  = fds_read_hook;
  mem->fds_write = fds_write_hook;

  /* Special connection to the PPU for mirroring: */
  fds->ppu = ppu;

  /* Timer: */
  fds->timer_irq_enable   = false;
  fds->timer_irq_repeat   = false;
  fds->timer_irq_occurred = false;
  fds->timer_count  = 0;
  fds->timer_reload = 0;

  /* Registers: */
  fds->ctrl               = 0;
  fds->io_enable_disk     = false;
  fds->io_enable_sound    = false;
  fds->disk_inserted      = false;
  fds->disk_ready         = false;
  fds->disk_write_protect = false;
  fds->trigger_irq        = false;
  fds->ack_disk_irq       = true;
  fds->ack_timer_irq      = true;
  fds->byte_transferred   = false;
  fds->data_read          = 0;

  /* RAM: */
  for (i = 0; i < FDS_RAM_SIZE; i++) {
    fds->ram[i] = 0xFF;
  }

  /* Disk image: */
  fds->image_state = IMAGE_STATE_BLOCK_1;
  fds->image_index        = 0;
  fds->image_block_offset = 0;
  fds->image_file_count   = 0;
  fds->image_file_size    = 0;
  fds->image_file_number  = 0;
  fds->image_crc_bytes    = 0;
  for (i = 0; i < FDS_IMAGE_SIZE; i++) {
    fds->image[i] = 0;
  }
  for (i = 0; i < FDS_FILE_INFO_MAX; i++) {
    fds->file_info[i].used    = false;
    fds->file_info[i].id      = 0;
    fds->file_info[i].size    = 0;
    fds->file_info[i].address = 0;
    fds->file_info[i].type    = 0;
    fds->file_info[i].name[0] = '\0';
    fds->file_info[i].name[8] = '\0';
  }
}



static uint8_t fds_image_read(fds_t *fds)
{
  uint8_t byte = 0; /* Default return, possibly dummy CRC byte. */

  switch (fds->image_state) {
  case IMAGE_STATE_BLOCK_1:
    byte = fds->image[fds->image_index];
    fds->image_index++;
    if (fds->image_index == 0x38) { /* Size of block 1 */
      fds->image_crc_bytes = 2;
      /* Kludge: Add 19 frames to keep possible FM2 TAS in sync. */
      fds->ppu->frame_no += 19;
      fds->image_state = IMAGE_STATE_BLOCK_1_CRC;
    }
    break;

  case IMAGE_STATE_BLOCK_1_CRC:
    fds->image_crc_bytes--;
    if (fds->image_crc_bytes == 0) {
      fds->image_state = IMAGE_STATE_BLOCK_2;
    }
    break;

  case IMAGE_STATE_BLOCK_2:
    byte = fds->image[fds->image_index];
    fds->image_index++;
    if (fds->image_index == 0x3A) { /* Size of block 1 + block 2 */
      fds->image_file_count = byte;
      fds->image_crc_bytes = 2;
      fds->image_state = IMAGE_STATE_BLOCK_2_CRC;
    }
    break;

  case IMAGE_STATE_BLOCK_2_CRC:
    fds->image_crc_bytes--;
    if (fds->image_crc_bytes == 0) {
      fds->image_block_offset = 0;
      fds->image_state = IMAGE_STATE_BLOCK_3;
    }
    break;

  case IMAGE_STATE_BLOCK_3:
    byte = fds->image[fds->image_index];
    fds->image_index++;

    switch (fds->image_block_offset) {
    case 0x01:
      fds->image_file_number = byte;
      fds->file_info[fds->image_file_number].used = true;
      break;
    case 0x02:
      fds->file_info[fds->image_file_number].id = byte;
      break;
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x0A:
      fds->file_info[fds->image_file_number].name
        [fds->image_block_offset - 3] = byte;
      break;
    case 0x0B:
      fds->file_info[fds->image_file_number].address = byte;
      break;
    case 0x0C:
      fds->file_info[fds->image_file_number].address += (byte * 256);
      break;
    case 0x0D:
      fds->image_file_size = byte;
      fds->file_info[fds->image_file_number].size = byte;
      break;
    case 0x0E:
      fds->image_file_size += (byte * 256);
      fds->file_info[fds->image_file_number].size += (byte * 256);
      break;
    case 0x0F:
      fds->file_info[fds->image_file_number].type = byte;
      fds->image_crc_bytes = 2;
      fds->image_state = IMAGE_STATE_BLOCK_3_CRC;
      break;
    default:
      break;
    }
    fds->image_block_offset++;
    break;

  case IMAGE_STATE_BLOCK_3_CRC:
    fds->image_crc_bytes--;
    if (fds->image_crc_bytes == 0) {
      fds->image_block_offset = 0;
      fds->image_state = IMAGE_STATE_BLOCK_4;
    }
    break;

  case IMAGE_STATE_BLOCK_4:
    byte = fds->image[fds->image_index];
    fds->image_index++;

    if (fds->image_block_offset >= fds->image_file_size) {
      fds->image_crc_bytes = 2;
      fds->image_state = IMAGE_STATE_BLOCK_4_CRC;
    }
    fds->image_block_offset++;
    break;

  case IMAGE_STATE_BLOCK_4_CRC:
    fds->image_crc_bytes--;
    if (fds->image_crc_bytes == 0) {
      if ((fds->image_file_number + 1) == fds->image_file_count) {
        fds->image_index = 0;
        fds->image_state = IMAGE_STATE_BLOCK_1;
      } else {
        fds->image_block_offset = 0;
        fds->image_state = IMAGE_STATE_BLOCK_3;
      }
    }
    break;

  default:
    break;
  }

  /* Reset to start of disk if end of image happens to be reached: */
  if (fds->image_index >= FDS_IMAGE_SIZE) {
    fds->image_index = 0;
    fds->image_state = IMAGE_STATE_BLOCK_1;
  }

  return byte;
}



void fds_execute(fds_t *fds)
{
  static int wait = FDS_WAIT_CYCLES;

  if (fds->irq_transfer && fds->ack_disk_irq) {
    if (wait > 0) {
      /* Need to wait for FDS BIOS to be ready. */
      wait--;
    } else {
      /* Now the byte can be transferred. */
      wait = FDS_WAIT_CYCLES;
      fds->ack_disk_irq = false;
      fds->trigger_irq = true;
      fds->timer_irq_occurred = false;
      fds->data_read = fds_image_read(fds);
      fds->byte_transferred = true;
    }
  }

  if (fds->timer_irq_enable) {
    if (fds->timer_count > 0) {
      fds->timer_count--;
    }
    if (fds->timer_count == 0 && fds->ack_timer_irq) {
      fds->ack_timer_irq = false;
      fds->trigger_irq = true;
      fds->timer_irq_occurred = true;
      fds->timer_count = fds->timer_reload;
      if (fds->timer_irq_repeat == false) {
        fds->timer_irq_enable = false;
      }
    }
  }
}



void fds_dump(FILE *fh, fds_t *fds)
{
  int i;

  fprintf(fh, "RAM:\n");
  for (i = 0; i < FDS_RAM_SIZE; i++) {
    if (i % 16 == 0) {
      fprintf(fh, "$%04x   ", i);
    }
    fprintf(fh, "%02x ", fds->ram[i]);
    if (i % 16 == 15) {
      fprintf(fh, "\n");
    }
  }

  fprintf(fh, "Timer:\n");
  fprintf(fh, "  IRQ Enable  : %d\n", fds->timer_irq_enable);
  fprintf(fh, "  IRQ Repeat  : %d\n", fds->timer_irq_repeat);
  fprintf(fh, "  IRQ Occurred: %d\n", fds->timer_irq_occurred);
  fprintf(fh, "  Count       : %d\n", fds->timer_count);
  fprintf(fh, "  Reload      : %d\n", fds->timer_reload);

  fprintf(fh, "Control           : 0x%02x\n", fds->ctrl);
  fprintf(fh, "  Drive Motor     : %d\n", fds->drive_motor);
  fprintf(fh, "  Transfer Reset  : %d\n", fds->transfer_reset);
  fprintf(fh, "  Read/Write Mode : %d (%s)\n", fds->rw_mode,
    (fds->rw_mode) ? "Read" : "Write");
  fprintf(fh, "  Mirroring       : %d (%s)\n", fds->mirroring,
    (fds->mirroring) ? "Horizontal" : "Vertical");
  fprintf(fh, "  CRC Control     : %d\n", fds->crc_control);
  fprintf(fh, "  Read/Write Start: %d\n", fds->rw_start);
  fprintf(fh, "  IRQ Transfer    : %d\n", fds->irq_transfer);

  fprintf(fh, "Drive Status:\n");
  fprintf(fh, "  Disk Inserted     : %d\n", fds->disk_inserted);
  fprintf(fh, "  Disk Ready        : %d\n", fds->disk_ready);
  fprintf(fh, "  Disk Write Protect: %d\n", fds->disk_write_protect);

  fprintf(fh, "Master I/O:\n");
  fprintf(fh, "  Disk I/O Enable : %d\n", fds->io_enable_disk);
  fprintf(fh, "  Sound I/O Enable: %d\n", fds->io_enable_sound);

  fprintf(fh, "Disk Image:\n");
  fprintf(fh, "  State            : %d\n", fds->image_state);
  fprintf(fh, "  Index            : %d\n", fds->image_index);
  fprintf(fh, "  Block Offset     : %d\n", fds->image_block_offset);
  fprintf(fh, "  Current File Size: %d\n", fds->image_file_size);
  fprintf(fh, "  CRC Bytes Left   : %d\n", fds->image_crc_bytes);

  fprintf(fh, "Ack Disk IRQ    : %d\n", fds->ack_disk_irq);
  fprintf(fh, "Ack Timer IRQ   : %d\n", fds->ack_timer_irq);
  fprintf(fh, "Byte Transferred: %d\n", fds->byte_transferred);

  fprintf(fh, "Files:\n");
  fprintf(fh, "  Count: %d\n", fds->image_file_count);
  fprintf(fh, "  No  ID  Name      Addr    Size   Type\n");
  for (i = 0; i < FDS_FILE_INFO_MAX; i++) {
    if (fds->file_info[i].used) {
      fprintf(fh, "  %-3d %-3d %s  0x%04x  %-5d  %-3d\n", i,
        fds->file_info[i].id,
        fds->file_info[i].name,
        fds->file_info[i].address,
        fds->file_info[i].size,
        fds->file_info[i].type);
    }
  }
}



