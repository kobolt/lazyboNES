#ifndef _FDS_H
#define _FDS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "mem.h"
#include "ppu.h"

#define FDS_RAM_SIZE 0x1800
#define FDS_IMAGE_SIZE 65500

#define FDS_FILE_INFO_MAX 256

typedef enum {
  IMAGE_STATE_BLOCK_1 = 0,
  IMAGE_STATE_BLOCK_1_CRC,
  IMAGE_STATE_BLOCK_2,
  IMAGE_STATE_BLOCK_2_CRC,
  IMAGE_STATE_BLOCK_3,
  IMAGE_STATE_BLOCK_3_CRC,
  IMAGE_STATE_BLOCK_4,
  IMAGE_STATE_BLOCK_4_CRC,
} fds_image_state_t;

typedef struct fds_file_info_s {
  bool used;
  uint8_t id;
  uint16_t size;
  uint16_t address;
  uint8_t type;
  char name[9];
} fds_file_info_t;

typedef struct fds_s {
  uint8_t ram[FDS_RAM_SIZE];

  bool timer_irq_enable;
  bool timer_irq_repeat;
  bool timer_irq_occurred;
  uint16_t timer_count;
  uint16_t timer_reload;

  bool io_enable_disk;
  bool io_enable_sound;
  bool disk_inserted;
  bool disk_ready;
  bool disk_write_protect;
  bool trigger_irq;
  bool ack_disk_irq;
  bool ack_timer_irq;
  bool byte_transferred;
  uint8_t data_read;

  union {
    struct {
      uint8_t drive_motor    : 1;
      uint8_t transfer_reset : 1;
      uint8_t rw_mode        : 1;
      uint8_t mirroring      : 1;
      uint8_t crc_control    : 1;
      uint8_t dummy          : 1;
      uint8_t rw_start       : 1;
      uint8_t irq_transfer   : 1;
    };
    uint8_t ctrl;
  };

  fds_image_state_t image_state;
  int image_index;
  int image_block_offset;
  uint8_t image_file_count;
  uint16_t image_file_size;
  uint8_t image_file_number;
  uint32_t image_crc_bytes;
  uint8_t image[FDS_IMAGE_SIZE];

  ppu_t *ppu;

  fds_file_info_t file_info[FDS_FILE_INFO_MAX];
} fds_t;

#define FDS_TIMER_IRQ_RELOAD_LO 0x4020
#define FDS_TIMER_IRQ_RELOAD_HI 0x4021
#define FDS_TIMER_IRQ_CONTROL   0x4022
#define FDS_MASTER_IO_ENABLE    0x4023
#define FDS_DATA_WRITE          0x4024
#define FDS_CONTROL             0x4025
#define FDS_EXT_CONNECTOR_WRITE 0x4026
#define FDS_DISK_STATUS         0x4030
#define FDS_DATA_READ           0x4031
#define FDS_DRIVE_STATUS        0x4032
#define FDS_EXT_CONNECTOR_READ  0x4033

void fds_init(fds_t *fds, mem_t *mem, ppu_t *ppu);
void fds_execute(fds_t *fds);
void fds_dump(FILE *fh, fds_t *fds);

int fds_bios_load(const char *filename, mem_t *mem);
int fds_image_load(const char *filename, fds_t *fds);

#endif /* _FDS_H */
