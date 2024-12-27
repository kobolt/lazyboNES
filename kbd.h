#ifndef _KBD_H
#define _KBD_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
  KBD_KEY_NONE = 0,
  KBD_KEY_1,
  KBD_KEY_2,
  KBD_KEY_3,
  KBD_KEY_4,
  KBD_KEY_5,
  KBD_KEY_6,
  KBD_KEY_7,
  KBD_KEY_8,
  KBD_KEY_9,
  KBD_KEY_0,
  KBD_KEY_A,
  KBD_KEY_B,
  KBD_KEY_C,
  KBD_KEY_D,
  KBD_KEY_E,
  KBD_KEY_F,
  KBD_KEY_G,
  KBD_KEY_H,
  KBD_KEY_I,
  KBD_KEY_J,
  KBD_KEY_K,
  KBD_KEY_L,
  KBD_KEY_M,
  KBD_KEY_N,
  KBD_KEY_O,
  KBD_KEY_P,
  KBD_KEY_Q,
  KBD_KEY_R,
  KBD_KEY_S,
  KBD_KEY_T,
  KBD_KEY_U,
  KBD_KEY_V,
  KBD_KEY_W,
  KBD_KEY_X,
  KBD_KEY_Y,
  KBD_KEY_Z,
  KBD_KEY_F1,
  KBD_KEY_F2,
  KBD_KEY_F3,
  KBD_KEY_F4,
  KBD_KEY_F5,
  KBD_KEY_F6,
  KBD_KEY_F7,
  KBD_KEY_F8,
  KBD_KEY_MINUS,
  KBD_KEY_CARET,
  KBD_KEY_YEN,
  KBD_KEY_STOP,
  KBD_KEY_ESC,
  KBD_KEY_AT,
  KBD_KEY_LEFT_BRACKET,
  KBD_KEY_RIGHT_BRACKET,
  KBD_KEY_RETURN,
  KBD_KEY_CTR,
  KBD_KEY_SEMICOLON,
  KBD_KEY_COLON,
  KBD_KEY_KANA,
  KBD_KEY_LEFT_SHIFT,
  KBD_KEY_RIGHT_SHIFT,
  KBD_KEY_COMMA,
  KBD_KEY_PERIOD,
  KBD_KEY_SLASH,
  KBD_KEY_UNDERSCORE,
  KBD_KEY_GRPH,
  KBD_KEY_SPACE,
  KBD_KEY_CLR_HOME,
  KBD_KEY_INS,
  KBD_KEY_DEL,
  KBD_KEY_UP,
  KBD_KEY_DOWN,
  KBD_KEY_LEFT,
  KBD_KEY_RIGHT,
} kbd_key_t;

typedef struct kbd_s {
  kbd_key_t key;
  bool shift;
  bool ctrl;
  int persist;

  uint32_t cassette_cycle;
  FILE *cassette_load_fh;
  FILE *cassette_save_fh;
  uint32_t cassette_save_sample_count;
} kbd_t;

void kbd_init(void);
void kbd_key_set(kbd_key_t key, bool shift, bool ctrl);
void kbd_key_clear(void);
uint8_t kbd_port_get(uint8_t row_counter, bool col_select);
void kbd_cassette_execute(bool dac, bool *adc);
int kbd_cassette_load_file(const char *filename);
int kbd_cassette_save_file_start(const char *filename);
int kbd_cassette_save_file_stop(void);

#endif /* _KBD_H */
