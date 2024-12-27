#include "kbd.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define KBD_CASSETTE_INTERNAL_SAMPLE_RATE 1789773 /* CPU Clock Speed */
#define KBD_CASSETTE_WAV_SAMPLE_RATE 44100

typedef struct wav_header_s {
  uint8_t riff_string[4];
  uint32_t chunk_size;
  uint8_t wave_string[4];
  uint8_t fmt_string[4];
  uint32_t subchunk1_size;
  uint16_t audio_format;
  uint16_t channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;
  uint8_t data_string[4];
  uint32_t subchunk2_size;
} wav_header_t;



static kbd_t kbd;



void kbd_init(void)
{
  memset(&kbd, 0, sizeof(kbd_t));
}



void kbd_key_set(kbd_key_t key, bool shift, bool ctrl)
{
  kbd.key = key;
  kbd.shift = shift;
  kbd.ctrl = ctrl;
  kbd.persist = 2;
}



void kbd_key_clear(void)
{
  if (kbd.persist > 0) {
    kbd.persist--;
  } else {
    kbd.key = KBD_KEY_NONE;
    kbd.shift = false;
    kbd.ctrl = false;
  }
}



uint8_t kbd_port_get(uint8_t row_counter, bool col_select)
{
  uint8_t port = 0xFF;

  switch (row_counter) {
  case 0:
    if (col_select == false) {
      if (kbd.key == KBD_KEY_RIGHT_BRACKET) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_LEFT_BRACKET) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_RETURN) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_F8) {
        port &= ~0x02;
      }
    } else {
      if (kbd.key == KBD_KEY_STOP) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_YEN) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_RIGHT_SHIFT || kbd.shift) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_KANA) {
        port &= ~0x02;
      }
    }
    break;

  case 1:
    if (col_select == false) {
      if (kbd.key == KBD_KEY_SEMICOLON) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_COLON) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_AT) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_F7) {
        port &= ~0x02;
      }
    } else {
      if (kbd.key == KBD_KEY_CARET) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_MINUS) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_SLASH) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_UNDERSCORE) {
        port &= ~0x02;
      }
    }
    break;

  case 2:
    if (col_select == false) {
      if (kbd.key == KBD_KEY_K) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_L) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_O) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_F6) {
        port &= ~0x02;
      }
    } else {
      if (kbd.key == KBD_KEY_0) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_P) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_COMMA) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_PERIOD) {
        port &= ~0x02;
      }
    }
    break;

  case 3:
    if (col_select == false) {
      if (kbd.key == KBD_KEY_J) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_U) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_I) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_F5) {
        port &= ~0x02;
      }
    } else {
      if (kbd.key == KBD_KEY_8) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_9) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_N) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_M) {
        port &= ~0x02;
      }
    }
    break;

  case 4:
    if (col_select == false) {
      if (kbd.key == KBD_KEY_H) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_G) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_Y) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_F4) {
        port &= ~0x02;
      }
    } else {
      if (kbd.key == KBD_KEY_6) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_7) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_V) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_B) {
        port &= ~0x02;
      }
    }
    break;

  case 5:
    if (col_select == false) {
      if (kbd.key == KBD_KEY_D) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_R) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_T) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_F3) {
        port &= ~0x02;
      }
    } else {
      if (kbd.key == KBD_KEY_4) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_5) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_C) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_F) {
        port &= ~0x02;
      }
    }
    break;

  case 6:
    if (col_select == false) {
      if (kbd.key == KBD_KEY_A) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_S) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_W) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_F2) {
        port &= ~0x02;
      }
    } else {
      if (kbd.key == KBD_KEY_3) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_E) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_Z) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_X) {
        port &= ~0x02;
      }
    }
    break;

  case 7:
    if (col_select == false) {
      if (kbd.key == KBD_KEY_CTR || kbd.ctrl) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_Q) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_ESC) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_F1) {
        port &= ~0x02;
      }
    } else {
      if (kbd.key == KBD_KEY_2) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_1) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_GRPH) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_LEFT_SHIFT) {
        port &= ~0x02;
      }
    }
    break;

  case 8:
    if (col_select == false) {
      if (kbd.key == KBD_KEY_LEFT) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_RIGHT) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_UP) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_CLR_HOME) {
        port &= ~0x02;
      }
    } else {
      if (kbd.key == KBD_KEY_INS) {
        port &= ~0x10;
      }
      if (kbd.key == KBD_KEY_DEL) {
        port &= ~0x08;
      }
      if (kbd.key == KBD_KEY_SPACE) {
        port &= ~0x04;
      }
      if (kbd.key == KBD_KEY_DOWN) {
        port &= ~0x02;
      }
    }
    break;

  default:
    break;
  }

  return port;
}



int kbd_cassette_load_file(const char *filename)
{
  wav_header_t header;

  if (kbd.cassette_load_fh != NULL) {
    return -2; /* Load already in progress. */
  }

  kbd.cassette_load_fh = fopen(filename, "rb");
  if (kbd.cassette_load_fh == NULL) {
    return -1; /* File not found. */
  }

  if (fread(&header, sizeof(wav_header_t), 1, kbd.cassette_load_fh) != 1) {
    fclose(kbd.cassette_load_fh);
    kbd.cassette_load_fh = NULL;
    return -3; /* Unable to read header. */
  }

  if (header.riff_string[0] != 'R') {
    fclose(kbd.cassette_load_fh);
    kbd.cassette_load_fh = NULL;
    return -4; /* Not a WAV file? */
  }

  if (header.sample_rate != KBD_CASSETTE_WAV_SAMPLE_RATE) {
    fclose(kbd.cassette_load_fh);
    kbd.cassette_load_fh = NULL;
    return -5; /* Unsupported sample rate. */
  }

  if (header.channels != 1) {
    fclose(kbd.cassette_load_fh);
    kbd.cassette_load_fh = NULL;
    return -6; /* Unsupported channels. */
  }

  if (header.bits_per_sample != 8) {
    fclose(kbd.cassette_load_fh);
    kbd.cassette_load_fh = NULL;
    return -7; /* Unsupported BPS. */
  }

  return 0;
}



int kbd_cassette_save_file_start(const char *filename)
{
  wav_header_t header;

  if (kbd.cassette_save_fh != NULL) {
    return -2; /* Save already in progress. */
  }

  kbd.cassette_save_fh = fopen(filename, "wbx");
  if (kbd.cassette_save_fh == NULL) {
    return -1; /* File not found. */
  }

  kbd.cassette_save_sample_count = 0;

  /* Prepare and write WAV header: */
  header.riff_string[0] = 'R';
  header.riff_string[1] = 'I';
  header.riff_string[2] = 'F';
  header.riff_string[3] = 'F';
  header.chunk_size = 0; /* Unknown until finished. */
  header.wave_string[0] = 'W';
  header.wave_string[1] = 'A';
  header.wave_string[2] = 'V';
  header.wave_string[3] = 'E';
  header.fmt_string[0] = 'f';
  header.fmt_string[1] = 'm';
  header.fmt_string[2] = 't';
  header.fmt_string[3] = ' ';
  header.subchunk1_size = 16;
  header.audio_format = 1; /* PCM */
  header.channels = 1; /* Mono */
  header.sample_rate = KBD_CASSETTE_WAV_SAMPLE_RATE;
  header.byte_rate = KBD_CASSETTE_WAV_SAMPLE_RATE;
  header.block_align = 1;
  header.bits_per_sample = 8;
  header.data_string[0] = 'd';
  header.data_string[1] = 'a';
  header.data_string[2] = 't';
  header.data_string[3] = 'a';
  header.subchunk2_size = 0; /* Unknown until finished. */

  fwrite(&header, sizeof(wav_header_t), 1, kbd.cassette_save_fh);
  return 0;
}



int kbd_cassette_save_file_stop(void)
{
  uint32_t chunk_size;
  uint32_t subchunk2_size;

  if (kbd.cassette_save_fh == NULL) {
    return -1; /* No file being saved. */
  }

  subchunk2_size = kbd.cassette_save_sample_count;
  chunk_size = subchunk2_size + 36;

  /* Update WAV header with chunk sizes before closing: */
  fseek(kbd.cassette_save_fh, 4, SEEK_SET);
  fwrite(&chunk_size, sizeof(uint32_t), 1, kbd.cassette_save_fh);
  fseek(kbd.cassette_save_fh, 40, SEEK_SET);
  fwrite(&subchunk2_size, sizeof(uint32_t), 1, kbd.cassette_save_fh);

  fclose(kbd.cassette_save_fh);
  kbd.cassette_save_fh = NULL;
  return 0;
}



void kbd_cassette_execute(bool dac, bool *adc)
{
  uint8_t sample;

  kbd.cassette_cycle++;

  if (kbd.cassette_save_fh != NULL) {
    if (kbd.cassette_cycle %
      (KBD_CASSETTE_INTERNAL_SAMPLE_RATE /
       KBD_CASSETTE_WAV_SAMPLE_RATE) == 0) {
      if (dac == true) {
        sample = UINT8_MAX;
      } else {
        sample = 0;
      }
      fwrite(&sample, sizeof(uint8_t), 1, kbd.cassette_save_fh);
      kbd.cassette_save_sample_count++;
    }
  }

  if (kbd.cassette_load_fh != NULL) {
    if (kbd.cassette_cycle %
      (KBD_CASSETTE_INTERNAL_SAMPLE_RATE /
       KBD_CASSETTE_WAV_SAMPLE_RATE) == 0) {

      if (fread(&sample, sizeof(uint8_t), 1, kbd.cassette_load_fh) != 1) {
        fclose(kbd.cassette_load_fh);
        kbd.cassette_load_fh = NULL;
      }

      if (sample > 128) {
        *adc = true;
      } else {
        *adc = false;
      }
    }
  }
}



