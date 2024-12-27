#ifndef _CLI_H
#define _CLI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef EXTRA_INFO
#include "mem.h"
#include "ppu.h"
#include "apu.h"
#endif

int cli_init(bool enable_colors, bool basic_mode);
void cli_draw_tile(uint8_t y, uint8_t x, bool table_no, uint8_t tile,
  uint8_t color_backdrop,
  uint8_t color_1,
  uint8_t color_2,
  uint8_t color_3,
  uint8_t color_4);
uint8_t cli_get_controller_state(void);
#ifdef SPECIAL_TERMINAL
void cli_audio_update(uint16_t freq, uint8_t volume);
#endif
#ifdef EXTRA_INFO
void cli_update(mem_t *mem, ppu_t *ppu, apu_t* apu);
#else
void cli_update(void);
#endif
void cli_pause(void);
void cli_resume(void);
int cli_text_inject(const char *filename);

#endif /* _CLI_H */
