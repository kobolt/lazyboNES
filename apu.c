#include "apu.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "mem.h"
#include "kbd.h"
#include "gui.h"
#include "cli.h"
#include "tas.h"
#include "panic.h"

static uint8_t apu_length_index[32] = {
  0x0A, 0xFE,
  0x14, 0x02,
  0x28, 0x04,
  0x50, 0x06,
  0xA0, 0x08,
  0x3C, 0x0A,
  0x0E, 0x0C,
  0x1A, 0x0E,
  0x0C, 0x10,
  0x18, 0x12,
  0x30, 0x14,
  0x60, 0x16,
  0xC0, 0x18,
  0x48, 0x1A,
  0x10, 0x1C,
  0x20, 0x1E,
};

static uint16_t apu_noise_period_index[16] = { /* NTSC values */
    4,    8,   16,   32,
   64,   96,  128,  160,
  202,  254,  380,  508,
  762, 1016, 2034, 4068,
};



static uint8_t apu_read_hook(void *apu, uint16_t address)
{
  uint8_t value;
  int controller;

  switch (address) {
  case APU_SND_CHN:
    value  =  ((apu_t *)apu)->pulse_length_counter[0] > 0 ? 1 : 0;
    value |= (((apu_t *)apu)->pulse_length_counter[1] > 0 ? 1 : 0) << 1;
    value |= (((apu_t *)apu)->triangle_length_counter > 0 ? 1 : 0) << 2;
    value |= (((apu_t *)apu)->noise_length_counter    > 0 ? 1 : 0) << 3;
    return value;

  case APU_JOY_1:
  case APU_JOY_2:
    controller = (address == APU_JOY_1) ? 0 : 1;
    if (((apu_t *)apu)->keyboard_enable) {
      if (controller == 1) { /* Keyboard */
        return ((apu_t *)apu)->keyboard_port;
      } else { /* Cassette */
        return ((apu_t *)apu)->keyboard_cassette_adc << 1;
      }
    }
    if (((apu_t *)apu)->controller[controller].strobe == true) {
      /* Return first bit ('A' Button) if strobe is high. */
      return ((apu_t *)apu)->controller[controller].data.a;
    } else {
      value = (((apu_t *)apu)->controller[controller].data.byte >>
        (((apu_t *)apu)->controller[controller].shift)) & 0x1;
      ((apu_t *)apu)->controller[controller].shift++;
      return value;
    }
    return 0;

  default:
    return 0; /* Other registers are write-only. */
  }
}



static void apu_write_hook(void *apu, uint16_t address, uint8_t value)
{
  switch (address) {
  case APU_SQ1_VOL:
    ((apu_t *)apu)->pulse_length_halt[0] = (value >> 5) & 0x1;
    ((apu_t *)apu)->pulse_envelope_loop[0] = (value >> 5) & 0x1;
    ((apu_t *)apu)->pulse_envelope_disable[0] = (value >> 4) & 0x1;
    ((apu_t *)apu)->pulse_envelope[0] = value & 0b1111;
    break;

  case APU_SQ2_VOL:
    ((apu_t *)apu)->pulse_length_halt[1] = (value >> 5) & 0x1;
    ((apu_t *)apu)->pulse_envelope_loop[1] = (value >> 5) & 0x1;
    ((apu_t *)apu)->pulse_envelope_disable[1] = (value >> 4) & 0x1;
    ((apu_t *)apu)->pulse_envelope[1] = value & 0b1111;
    break;

  case APU_SQ1_SWEEP:
    ((apu_t *)apu)->pulse_sweep_enable[0] = value >> 7;
    ((apu_t *)apu)->pulse_sweep_period[0] = (value >> 4) & 0x7;
    ((apu_t *)apu)->pulse_sweep_negate[0] = (value >> 3) & 0x1;
    ((apu_t *)apu)->pulse_sweep_shift[0] = value & 0x7;
    ((apu_t *)apu)->pulse_sweep_reload[0] = true;
    break;

  case APU_SQ2_SWEEP:
    ((apu_t *)apu)->pulse_sweep_enable[1] = value >> 7;
    ((apu_t *)apu)->pulse_sweep_period[1] = (value >> 4) & 0x7;
    ((apu_t *)apu)->pulse_sweep_negate[1] = (value >> 3) & 0x1;
    ((apu_t *)apu)->pulse_sweep_shift[1] = value & 0x7;
    ((apu_t *)apu)->pulse_sweep_reload[1] = true;
    break;

  case APU_SQ1_LO:
    ((apu_t *)apu)->pulse_timer[0] = 
      (((apu_t *)apu)->pulse_timer[0] & 0xFF00) + value;
    break;

  case APU_SQ2_LO:
    ((apu_t *)apu)->pulse_timer[1] = 
      (((apu_t *)apu)->pulse_timer[1] & 0xFF00) + value;
    break;

  case APU_SQ1_HI:
    ((apu_t *)apu)->pulse_timer[0] = 
      (((apu_t *)apu)->pulse_timer[0] & 0xFF) + ((value & 0b111) << 8);
    ((apu_t *)apu)->pulse_length_counter[0] = apu_length_index[value >> 3];
    ((apu_t *)apu)->pulse_envelope_reset[0] = true;
    break;

  case APU_SQ2_HI:
    ((apu_t *)apu)->pulse_timer[1] = 
      (((apu_t *)apu)->pulse_timer[1] & 0xFF) + ((value & 0b111) << 8);
    ((apu_t *)apu)->pulse_length_counter[1] = apu_length_index[value >> 3];
    ((apu_t *)apu)->pulse_envelope_reset[1] = true;
    break;

  case APU_TRI_LINEAR:
    ((apu_t *)apu)->triangle_control = value >> 7;
    ((apu_t *)apu)->triangle_length_halt = value >> 7;
    ((apu_t *)apu)->triangle_reload_value = value & 0b1111111;
    break;

  case APU_TRI_LO:
    ((apu_t *)apu)->triangle_timer = 
      (((apu_t *)apu)->triangle_timer & 0xFF00) + value;
    break;

  case APU_TRI_HI:
    ((apu_t *)apu)->triangle_timer =
      (((apu_t *)apu)->triangle_timer & 0xFF) + ((value & 0b111) << 8);
    ((apu_t *)apu)->triangle_length_counter = apu_length_index[value >> 3];
    ((apu_t *)apu)->triangle_linear_halt = true;
    break;

  case APU_NOISE_VOL:
    ((apu_t *)apu)->noise_length_halt = (value >> 5) & 0x1;
    ((apu_t *)apu)->noise_envelope_loop = (value >> 5) & 0x1;
    ((apu_t *)apu)->noise_envelope_disable = (value >> 4) & 0x1;
    ((apu_t *)apu)->noise_envelope = value & 0b1111;
    break;

  case APU_NOISE_LO:
    ((apu_t *)apu)->noise_mode = value >> 7;
    ((apu_t *)apu)->noise_period = apu_noise_period_index[value & 0b1111];
    break;

  case APU_NOISE_HI:
    ((apu_t *)apu)->noise_length_counter = apu_length_index[value >> 3];
    ((apu_t *)apu)->noise_envelope_reset = true;
    break;

  case APU_DMC_FREQ:
  case APU_DMC_RAW:
  case APU_DMC_START:
  case APU_DMC_LEN:
    /* Not implemented. */
    break;

  case APU_SND_CHN:
    ((apu_t *)apu)->pulse_enable[0] =  value       & 0x1;
    ((apu_t *)apu)->pulse_enable[1] = (value >> 1) & 0x1;
    ((apu_t *)apu)->triangle_enable = (value >> 2) & 0x1;
    ((apu_t *)apu)->noise_enable    = (value >> 3) & 0x1;
    /* Clear length counters if disabled. */
    if (((apu_t *)apu)->pulse_enable[0] == 0) {
      ((apu_t *)apu)->pulse_length_counter[0] = 0;
    }
    if (((apu_t *)apu)->pulse_enable[1] == 0) {
      ((apu_t *)apu)->pulse_length_counter[1] = 0;
    }
    if (((apu_t *)apu)->triangle_enable == 0) {
      ((apu_t *)apu)->triangle_length_counter = 0;
    }
    if (((apu_t *)apu)->noise_enable == 0) {
      ((apu_t *)apu)->noise_length_counter = 0;
    }
    break;

  case APU_JOY_1:
    ((apu_t *)apu)->controller[0].strobe = value & 1;
    ((apu_t *)apu)->controller[1].strobe = value & 1;
    ((apu_t *)apu)->keyboard_enable = value & 4;
    if (value & 4) {
      if ((((apu_t *)apu)->keyboard_col_select == true) && (value & 2) == 0) {
        /* Increment row counter on high->low transition. */
        ((apu_t *)apu)->keyboard_row_counter++;
      }
      ((apu_t *)apu)->keyboard_col_select = (value & 2) >> 1;
    }
    if (value & 1) {
      /* Reset shift registers. */
      ((apu_t *)apu)->controller[0].shift = 0;
      ((apu_t *)apu)->controller[1].shift = 0;
      ((apu_t *)apu)->keyboard_row_counter = 0;
      ((apu_t *)apu)->keyboard_cassette_dac = true;
    } else {
      ((apu_t *)apu)->keyboard_cassette_dac = false;
    }
    ((apu_t *)apu)->keyboard_port = kbd_port_get(
      ((apu_t *)apu)->keyboard_row_counter,
      ((apu_t *)apu)->keyboard_col_select);
    break;

  case APU_FRAME_CNT:
    ((apu_t *)apu)->sequencer_mode5 = (value >> 7);
    ((apu_t *)apu)->sequencer_divider = 0;
    ((apu_t *)apu)->sequencer_step = 0;
    break;

  default:
    panic("APU write (0x%02x) on unhandled address: 0x%04x\n", value, address);
    break;
  }
}



void apu_init(apu_t *apu, mem_t *mem)
{
  int i;

  /* Memory connections: */
  mem->apu = apu;
  mem->apu_read  = apu_read_hook;
  mem->apu_write = apu_write_hook;

  /* Controller state: */
  for (i = 0; i < APU_CONTROLLERS; i++) {
    apu->controller[i].data.byte = 0;
    apu->controller[i].strobe = false;
    apu->controller[i].shift = 0;
  }

  /* Sound registers: */
  apu->sequencer_mode5 = false;
  apu->sequencer_divider = 0;
  apu->sequencer_step = 0;

  /* Pulse/Square generators: */
  for (i = 0; i < 2; i++) {
    apu->pulse_enable[i]           = false;
    apu->pulse_timer[i]            = 0;
    apu->pulse_length_counter[i]   = 0;
    apu->pulse_envelope[i]         = 0;
    apu->pulse_envelope_counter[i] = 0;
    apu->pulse_envelope_divider[i] = 0;
    apu->pulse_length_halt[i]      = false;
    apu->pulse_envelope_loop[i]    = false;
    apu->pulse_envelope_disable[i] = false;
    apu->pulse_envelope_reset[i]   = false;
    apu->pulse_sweep_period[i]     = 0;
    apu->pulse_sweep_shift[i]      = 0;
    apu->pulse_sweep_counter[i]    = 0;
    apu->pulse_sweep_enable[i]     = false;
    apu->pulse_sweep_negate[i]     = false;
    apu->pulse_sweep_reload[i]     = false;
  }

  /* Triangle generator: */
  apu->triangle_enable         = false;
  apu->triangle_timer          = 0;
  apu->triangle_length_counter = 0;
  apu->triangle_linear_counter = 0;
  apu->triangle_reload_value   = 0;
  apu->triangle_length_halt    = false;
  apu->triangle_linear_halt    = false;
  apu->triangle_control        = false;

  /* Noise generator: */
  apu->noise_enable           = false;
  apu->noise_mode             = false;
  apu->noise_period           = 0;
  apu->noise_length_counter   = 0;
  apu->noise_envelope         = 0;
  apu->noise_envelope_counter = 0;
  apu->noise_envelope_divider = 0;
  apu->noise_length_halt      = false;
  apu->noise_envelope_loop    = false;
  apu->noise_envelope_disable = false;
  apu->noise_envelope_reset   = false;
}



static void apu_length_update(apu_t *apu)
{
  int i;

  for (i = 0; i < 2; i++) {
    if (apu->pulse_length_halt[i] == false) {
      if (apu->pulse_length_counter[i] > 0) {
        apu->pulse_length_counter[i]--;
      }
    } else {
      apu->pulse_length_counter[i] = 0;
    }
  }

  if (apu->triangle_length_halt == false) {
    if (apu->triangle_length_counter > 0) {
      apu->triangle_length_counter--;
    }
  } else {
    apu->triangle_length_counter = 0;
  }

  if (apu->noise_length_halt == false) {
    if (apu->noise_length_counter > 0) {
      apu->noise_length_counter--;
    }
  } else {
    apu->noise_length_counter = 0;
  }
}



static void apu_envelope_update(apu_t *apu)
{
  int i;

  if (apu->triangle_linear_halt == true) {
    apu->triangle_linear_counter = apu->triangle_reload_value;
  } else {
    if (apu->triangle_linear_counter > 0) {
      apu->triangle_linear_counter--;
    }
  }
  if (apu->triangle_control == false) {
    apu->triangle_linear_halt = false;
  }

  for (i = 0; i < 2; i++) {
    if (apu->pulse_envelope_reset[i] == true) {
      apu->pulse_envelope_reset[i] = false;
      apu->pulse_envelope_counter[i] = 15;
      apu->pulse_envelope_divider[i] = 0;

    } else {
      apu->pulse_envelope_divider[i]++;
      if (apu->pulse_envelope_divider[i] >= (apu->pulse_envelope[i] + 1)) {
        apu->pulse_envelope_divider[i] = 0;
        if (apu->pulse_envelope_loop[i] == true) {
          if (apu->pulse_envelope_counter[i] > 0) {
            apu->pulse_envelope_counter[i]--;
          } else {
            apu->pulse_envelope_counter[i] = 15;
          }
        }
      }
    }
  }

  if (apu->noise_envelope_reset == true) {
    apu->noise_envelope_reset = false;
    apu->noise_envelope_counter = 15;
    apu->noise_envelope_divider = 0;

  } else {
    apu->noise_envelope_divider++;
    if (apu->noise_envelope_divider >= (apu->noise_envelope + 1)) {
      apu->noise_envelope_divider = 0;
      if (apu->noise_envelope_loop == true) {
        if (apu->noise_envelope_counter > 0) {
          apu->noise_envelope_counter--;
        } else {
          apu->noise_envelope_counter = 15;
        }
      }
    }
  }
}



static void apu_sweep_update(apu_t *apu)
{
  int i;
  int change_amount;

  for (i = 0; i < 2; i++) {
    if (apu->pulse_sweep_counter[i] == 0 &&
        apu->pulse_sweep_enable[i] == true) {

      change_amount = apu->pulse_timer[i] >> apu->pulse_sweep_shift[i];
      if (change_amount < 0 && i == 0) {
        change_amount--; /* One's complement on channel #1 */
      }

      if (apu->pulse_sweep_negate[i] == true) {
        apu->pulse_timer[i] -= change_amount;
      } else {
        apu->pulse_timer[i] += change_amount;
      }
    }

    if (apu->pulse_sweep_counter[i] == 0 ||
      apu->pulse_sweep_reload[i] == true) {
      apu->pulse_sweep_reload[i] = false;
      apu->pulse_sweep_counter[i] = apu->pulse_sweep_period[i];
    } else {
      apu->pulse_sweep_counter[i]--;
    }
  }
}



void apu_execute(apu_t *apu)
{
  uint8_t volume;
  int i;

  /* Data is only fed into controller #1 currently. */
  if (tas_is_active()) {
    apu->controller[0].data.byte = tas_get_controller_state();
  } else {
    apu->controller[0].data.byte = gui_get_controller_state() |
                                   cli_get_controller_state();
  }

  /* Runs at 240Hz */
  apu->sequencer_divider++;
  if (apu->sequencer_divider > 7456) {
    apu->sequencer_divider = 0;

    switch (apu->sequencer_step) {
    case 0:
      if (apu->sequencer_mode5 == true) {
        apu_length_update(apu);
        apu_sweep_update(apu);
      }
      apu_envelope_update(apu);
      break;

    case 1:
      if (apu->sequencer_mode5 == false) {
        apu_length_update(apu);
        apu_sweep_update(apu);
      }
      apu_envelope_update(apu);
      break;

    case 2:
      if (apu->sequencer_mode5 == true) {
        apu_length_update(apu);
        apu_sweep_update(apu);
      }
      apu_envelope_update(apu);
      break;

    case 3:
      if (apu->sequencer_mode5 == false) {
        apu_length_update(apu);
        apu_sweep_update(apu);
      }
      apu_envelope_update(apu);
      break;

    case 4:
    default:
      break;
    }

    for (i = 0; i < 2; i++) {
      if (apu->pulse_enable[i] &&
        apu->pulse_timer[i] >= 8 &&
        apu->pulse_length_counter[i] > 0) {
        if (apu->pulse_envelope_disable[i] == true) {
          volume = apu->pulse_envelope[i] * 16;
        } else {
          volume = apu->pulse_envelope_counter[i] * 16;
        }
        gui_audio_square_update(i, 
          1789773 / (16 * (apu->pulse_timer[i] + 1)), volume);
#ifdef SPECIAL_TERMINAL
        if (i == 1) {
          cli_audio_update(1789773 / (16 * (apu->pulse_timer[1] + 1)), volume);
        }
#endif

      } else {
        gui_audio_square_update(i, 0, 0);
#ifdef SPECIAL_TERMINAL
        if (i == 1) {
          cli_audio_update(0, 0);
        }
#endif
      }
    }

    if (apu->triangle_enable &&
      apu->triangle_length_counter > 0 &&
      apu->triangle_linear_counter > 0) {
      gui_audio_triangle_update(1789773 / (32 * (apu->triangle_timer + 1)));

    } else {
      gui_audio_triangle_update(0);
    }

    if (apu->noise_enable &&
      apu->noise_length_counter > 0) {
      if (apu->noise_envelope_disable == true) {
        volume = apu->noise_envelope * 16;
      } else {
        volume = apu->noise_envelope_counter * 16;
      }
      gui_audio_noise_update(1789773 / (32 * (apu->noise_period + 1)), volume);

    } else {
      gui_audio_noise_update(0, 0);
    }

    apu->sequencer_step++;
    if (apu->sequencer_step > ((apu->sequencer_mode5 == true) ? 5 : 4)) {
      apu->sequencer_step = 0;
    }
  }
}



void apu_dump(FILE *fh, apu_t *apu)
{
  int i;

  for (i = 0; i < APU_CONTROLLERS; i++) {
    fprintf(fh, "Controller #%d: 0x%02x >> %d (%d)\n", i + 1,
      apu->controller[i].data.byte,
      apu->controller[i].shift,
      apu->controller[i].strobe);
  }

  fprintf(fh, "Keyboard\n");
  fprintf(fh, "  Enable       : %d\n", apu->keyboard_enable);
  fprintf(fh, "  Column Select: %d\n", apu->keyboard_col_select);
  fprintf(fh, "  Row Counter  : %d\n", apu->keyboard_row_counter);

  fprintf(fh, "Sequencer Mode: %d-step\n", 
    (apu->sequencer_mode5 == true) ? 5 : 4);
  fprintf(fh, "Sequencer Divider/Step: %d/%d\n",
    apu->sequencer_divider, apu->sequencer_step);

  for (i = 0; i < 2; i++) {
    fprintf(fh, "Pulse #%d\n", i);
    fprintf(fh, "  Enable          : %d\n", apu->pulse_enable[i]);
    fprintf(fh, "  Timer           : 0x%04x\n", apu->pulse_timer[i]);
    fprintf(fh, "  Length Counter  : %d\n", apu->pulse_length_counter[i]);
    fprintf(fh, "  Length Halt     : %d\n", apu->pulse_length_halt[i]);
    fprintf(fh, "  Envelope        : %d\n", apu->pulse_envelope[i]);
    fprintf(fh, "  Envelope Counter: %d\n", apu->pulse_envelope_counter[i]);
    fprintf(fh, "  Envelope Divider: %d\n", apu->pulse_envelope_divider[i]);
    fprintf(fh, "  Envelope Loop   : %d\n", apu->pulse_envelope_loop[i]);
    fprintf(fh, "  Envelope Disable: %d\n", apu->pulse_envelope_disable[i]);
    fprintf(fh, "  Sweep Period    : %d\n", apu->pulse_sweep_period[i]);
    fprintf(fh, "  Sweep Shift     : %d\n", apu->pulse_sweep_shift[i]);
    fprintf(fh, "  Sweep Counter   : %d\n", apu->pulse_sweep_counter[i]);
    fprintf(fh, "  Sweep Enable    : %d\n", apu->pulse_sweep_enable[i]);
    fprintf(fh, "  Sweep Negate    : %d\n", apu->pulse_sweep_negate[i]);
    fprintf(fh, "  Sweep Reload    : %d\n", apu->pulse_sweep_reload[i]);
  }

  fprintf(fh, "Triangle\n");
  fprintf(fh, "  Enable          : %d\n", apu->triangle_enable);
  fprintf(fh, "  Timer           : 0x%04x\n", apu->triangle_timer);
  fprintf(fh, "  Length Counter  : %d\n", apu->triangle_length_counter);
  fprintf(fh, "  Length Halt     : %d\n", apu->triangle_length_halt);
  fprintf(fh, "  Linear Counter  : %d\n", apu->triangle_linear_counter);
  fprintf(fh, "  Linear Halt     : %d\n", apu->triangle_linear_halt);
  fprintf(fh, "  Reload Value    : %d\n", apu->triangle_reload_value);

  fprintf(fh, "Noise\n");
  fprintf(fh, "  Enable          : %d\n", apu->noise_enable);
  fprintf(fh, "  Mode            : %d\n", apu->noise_mode);
  fprintf(fh, "  Period          : %d\n", apu->noise_period);
  fprintf(fh, "  Length Counter  : %d\n", apu->noise_length_counter);
  fprintf(fh, "  Length Halt     : %d\n", apu->noise_length_halt);
  fprintf(fh, "  Envelope        : %d\n", apu->noise_envelope);
  fprintf(fh, "  Envelope Counter: %d\n", apu->noise_envelope_counter);
  fprintf(fh, "  Envelope Divider: %d\n", apu->noise_envelope_divider);
  fprintf(fh, "  Envelope Loop   : %d\n", apu->noise_envelope_loop);
  fprintf(fh, "  Envelope Disable: %d\n", apu->noise_envelope_disable);
}



