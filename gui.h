#ifndef _GUI_H
#define _GUI_H

#include <stdint.h>
#include <stdbool.h>

int gui_init(int joystick_no, bool disable_video, bool disable_audio,
  bool basic_mode);
void gui_draw_scanline(uint16_t y, uint8_t colors[]);
uint8_t gui_get_controller_state(void);
void gui_audio_square_update(int channel, uint16_t freq, uint8_t volume);
void gui_audio_triangle_update(uint16_t freq);
void gui_audio_noise_update(uint16_t freq, uint8_t volume);
void gui_update(void);
bool gui_save_state_requested(void);
bool gui_load_state_requested(void);
void gui_warp_mode_set(bool value);
bool gui_warp_mode_get(void);

#endif /* _GUI_H */
