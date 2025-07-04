#ifndef _TUI_H
#define _TUI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef EXTRA_INFO
#include "mem.h"
#include "ppu.h"
#include "apu.h"
#endif

int tui_init(bool enable_colors, bool basic_mode);
void tui_draw_tile(uint8_t y, uint8_t x, bool table_no, uint8_t tile,
  uint8_t color_backdrop,
  uint8_t color_1,
  uint8_t color_2,
  uint8_t color_3,
  uint8_t color_4);
uint8_t tui_get_controller_state(void);
#ifdef SPECIAL_TERMINAL
void tui_audio_update(uint16_t freq, uint8_t volume);
#endif
#ifdef EXTRA_INFO
void tui_update(mem_t *mem, ppu_t *ppu, apu_t* apu);
#else
void tui_update(void);
#endif
void tui_pause(void);
void tui_resume(void);
int tui_text_inject(const char *filename);

#endif /* _TUI_H */
