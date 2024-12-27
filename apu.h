#ifndef _APU_H
#define _APU_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "mem.h"

#define APU_CONTROLLERS 2

typedef struct controller_s {
  union {
    struct {
      uint8_t a      : 1;
      uint8_t b      : 1;
      uint8_t select : 1;
      uint8_t start  : 1;
      uint8_t up     : 1;
      uint8_t down   : 1;
      uint8_t left   : 1;
      uint8_t right  : 1;
    };
    uint8_t byte;
  } data;
  bool strobe;
  uint8_t shift;
} controller_t;

typedef struct apu_s {
  controller_t controller[APU_CONTROLLERS];

  bool keyboard_enable;
  bool keyboard_col_select;
  uint8_t keyboard_row_counter;
  uint8_t keyboard_port;
  bool keyboard_cassette_dac;
  bool keyboard_cassette_adc;

  bool sequencer_mode5;
  uint16_t sequencer_divider;
  uint8_t sequencer_step;

  bool pulse_enable[2];
  uint16_t pulse_timer[2];
  uint16_t pulse_length_counter[2];
  uint8_t pulse_envelope[2];
  uint8_t pulse_envelope_counter[2];
  uint8_t pulse_envelope_divider[2];
  bool pulse_length_halt[2];
  bool pulse_envelope_loop[2];
  bool pulse_envelope_disable[2];
  bool pulse_envelope_reset[2];

  uint8_t pulse_sweep_period[2];
  uint8_t pulse_sweep_shift[2];
  uint8_t pulse_sweep_counter[2];
  bool pulse_sweep_enable[2];
  bool pulse_sweep_negate[2];
  bool pulse_sweep_reload[2];

  bool triangle_enable;
  uint16_t triangle_timer;
  uint16_t triangle_length_counter;
  uint16_t triangle_linear_counter;
  uint8_t triangle_reload_value;
  bool triangle_length_halt;
  bool triangle_linear_halt;
  bool triangle_control;

  bool noise_enable;
  bool noise_mode;
  uint16_t noise_period;
  uint16_t noise_length_counter;
  uint8_t noise_envelope;
  uint8_t noise_envelope_counter;
  uint8_t noise_envelope_divider;
  bool noise_length_halt;
  bool noise_envelope_loop;
  bool noise_envelope_disable;
  bool noise_envelope_reset;
} apu_t;

#define APU_SQ1_VOL    0x4000
#define APU_SQ1_SWEEP  0x4001
#define APU_SQ1_LO     0x4002
#define APU_SQ1_HI     0x4003
#define APU_SQ2_VOL    0x4004
#define APU_SQ2_SWEEP  0x4005
#define APU_SQ2_LO     0x4006
#define APU_SQ2_HI     0x4007
#define APU_TRI_LINEAR 0x4008
#define APU_TRI_LO     0x400A
#define APU_TRI_HI     0x400B
#define APU_NOISE_VOL  0x400C
#define APU_NOISE_LO   0x400E
#define APU_NOISE_HI   0x400F
#define APU_DMC_FREQ   0x4010
#define APU_DMC_RAW    0x4011
#define APU_DMC_START  0x4012
#define APU_DMC_LEN    0x4013
#define APU_OAM_DMA    0x4014
#define APU_SND_CHN    0x4015
#define APU_JOY_1      0x4016
#define APU_JOY_2      0x4017
#define APU_FRAME_CNT  0x4017

void apu_init(apu_t *apu, mem_t *mem);
void apu_execute(apu_t *apu);
void apu_dump(FILE *fh, apu_t *apu);

#endif /* _APU_H */
