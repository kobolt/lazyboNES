#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <math.h>
#include <time.h>

#define GUI_WIDTH 256
#define GUI_HEIGHT 240

#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_VOLUME 64 /* 0 -> 127 */

static SDL_Window *gui_window = NULL;
static SDL_Renderer *gui_renderer = NULL;
static SDL_Texture *gui_texture = NULL;
static SDL_Joystick *gui_joystick = NULL;
static SDL_PixelFormat *gui_pixel_format = NULL;
static Uint32 *gui_pixels = NULL;
static int gui_pixel_pitch = 0;
static Uint32 gui_ticks = 0;

static uint8_t gui_controller_state = 0;
static bool gui_save_state_request = false;
static bool gui_load_state_request = false;

static const uint8_t gui_sys_palette[64][3] =
{
  {0x52, 0x52, 0x52},
  {0x01, 0x1a, 0x51},
  {0x0f, 0x0f, 0x65},
  {0x23, 0x06, 0x63},
  {0x36, 0x03, 0x4b},
  {0x40, 0x04, 0x26},
  {0x3f, 0x09, 0x04},
  {0x32, 0x13, 0x00},
  {0x1f, 0x20, 0x00},
  {0x0b, 0x2a, 0x00},
  {0x00, 0x2f, 0x00},
  {0x00, 0x2e, 0x0a},
  {0x00, 0x26, 0x2d},
  {0x00, 0x00, 0x00},
  {0x00, 0x00, 0x00},
  {0x00, 0x00, 0x00},
  {0xa0, 0xa0, 0xa0},
  {0x1e, 0x4a, 0x9d},
  {0x38, 0x37, 0xbc},
  {0x58, 0x28, 0xb8},
  {0x75, 0x21, 0x94},
  {0x84, 0x23, 0x5c},
  {0x82, 0x2e, 0x24},
  {0x6f, 0x3f, 0x00},
  {0x51, 0x52, 0x00},
  {0x31, 0x63, 0x00},
  {0x1a, 0x6b, 0x05},
  {0x0e, 0x69, 0x2e},
  {0x10, 0x5c, 0x68},
  {0x00, 0x00, 0x00},
  {0x00, 0x00, 0x00},
  {0x00, 0x00, 0x00},
  {0xfe, 0xff, 0xff},
  {0x69, 0x9e, 0xfc},
  {0x89, 0x87, 0xff},
  {0xae, 0x76, 0xff},
  {0xce, 0x6d, 0xf1},
  {0xe0, 0x70, 0xb2},
  {0xde, 0x7c, 0x70},
  {0xc8, 0x91, 0x3e},
  {0xa6, 0xa7, 0x25},
  {0x81, 0xba, 0x28},
  {0x63, 0xc4, 0x46},
  {0x54, 0xc1, 0x7d},
  {0x56, 0xb3, 0xc0},
  {0x3c, 0x3c, 0x3c},
  {0x00, 0x00, 0x00},
  {0x00, 0x00, 0x00},
  {0xfe, 0xff, 0xff},
  {0xbe, 0xd6, 0xfd},
  {0xcc, 0xcc, 0xff},
  {0xdd, 0xc4, 0xff},
  {0xea, 0xc0, 0xf9},
  {0xf2, 0xc1, 0xdf},
  {0xf1, 0xc7, 0xc2},
  {0xe8, 0xd0, 0xaa},
  {0xd9, 0xda, 0x9d},
  {0xc9, 0xe2, 0x9e},
  {0xbc, 0xe6, 0xae},
  {0xb4, 0xe5, 0xc7},
  {0xb5, 0xdf, 0xe4},
  {0xa9, 0xa9, 0xa9},
  {0x00, 0x00, 0x00},
  {0x00, 0x00, 0x00},
};



static uint16_t audio_square_freq[2];
static uint8_t audio_square_volume[2];
static uint16_t audio_triangle_freq;
static uint16_t audio_noise_freq;
static uint8_t audio_noise_volume;

static inline double normalize(double phase)
{
  double cycles;
  cycles = phase / (2.0 * M_PI);
  phase -= trunc(cycles) * 2.0 * M_PI;
  if (phase < 0) {
    phase += 2.0 * M_PI;
  }
  return phase;
}

static inline double square(double phase)
{
  if (normalize(phase) < M_PI) {
    return 1.0;
  } else {
    return -1.0;
  }
}

static inline double triangle(double phase)
{
  phase = normalize(phase);
  if (phase >= M_PI) {
    phase = (2.0 * M_PI) - phase;
  }
  return -1.0 + ((2.0 * phase) / M_PI);
}

static inline double noise(double phase)
{
  (void)phase;
  return -1.0 + ((rand() % 1024) / 512.0);
}



static void audio_callback(void *userdata, Uint8 *stream, int len)
{
  static uint32_t sample_no = 0;
  int i;
  double sample;
  double time;
  (void)userdata;

#ifdef F32_AUDIO
  float *fstream = (float *)stream;
  for (i = 0; i < len / 4; i++) {
#else
  for (i = 0; i < len; i++) {
#endif /* F32_AUDIO */
    time = sample_no / (double)AUDIO_SAMPLE_RATE;
    sample = 0;

    if (audio_square_freq[0] > 0) {
      sample += (square(2.0 * M_PI * audio_square_freq[0] * time) / 256) * 
        audio_square_volume[0];
    }

    if (audio_square_freq[1] > 0) {
      sample += (square(2.0 * M_PI * audio_square_freq[1] * time) / 256) * 
        audio_square_volume[1];
    }

    if (audio_triangle_freq > 0) {
      sample += triangle(2.0 * M_PI * audio_triangle_freq * time);
    }

    if (audio_noise_freq > 0) {
      sample += (noise(2.0 * M_PI * audio_noise_freq * time) / 256) * 
        audio_noise_volume;
    }

    sample /= 4;

#ifdef F32_AUDIO
    fstream[i] = (float)sample * (AUDIO_VOLUME / 128.0);
#else
    stream[i] = (Uint8)(127 + (sample * AUDIO_VOLUME));
#endif /* F32_AUDIO */
    sample_no++;
  }
}



static int gui_audio_init(void)
{
  SDL_AudioSpec desired, obtained;

  audio_square_freq[0]   = 0;
  audio_square_freq[1]   = 0;
  audio_square_volume[0] = 0;
  audio_square_volume[1] = 0;
  audio_triangle_freq    = 0;
  audio_noise_freq       = 0;
  audio_noise_volume     = 0;

  desired.freq     = AUDIO_SAMPLE_RATE;
#ifdef F32_AUDIO
  desired.format   = AUDIO_F32LSB;
#else
  desired.format   = AUDIO_U8;
#endif /* F32_AUDIO */
  desired.channels = 1;
  desired.samples  = 2048; /* Buffer size */
  desired.userdata = 0;
  desired.callback = audio_callback;

  srand(time(NULL));

  if (SDL_OpenAudio(&desired, &obtained) != 0) {
    fprintf(stderr, "SDL_OpenAudio() failed: %s\n", SDL_GetError());
    return -1;
  }

#ifdef F32_AUDIO
  if (obtained.format != AUDIO_F32LSB) {
    fprintf(stderr, "Did not get float 32-bit audio format!\n");
#else
  if (obtained.format != AUDIO_U8) {
    fprintf(stderr, "Did not get unsigned 8-bit audio format!\n");
#endif /* F32_AUDIO */
    SDL_CloseAudio();
    return -1;
  }

  SDL_PauseAudio(0);
  return 0;
}



void gui_audio_square_update(int channel, uint16_t freq, uint8_t volume)
{
  audio_square_freq[channel]   = freq;
  audio_square_volume[channel] = volume;
}



void gui_audio_triangle_update(uint16_t freq)
{
  audio_triangle_freq = freq;
}



void gui_audio_noise_update(uint16_t freq, uint8_t volume)
{
  audio_noise_freq   = freq;
  audio_noise_volume = volume;
}



static void gui_exit_handler(void)
{
  SDL_PauseAudio(1);
  SDL_CloseAudio();

  if (SDL_JoystickGetAttached(gui_joystick)) {
    SDL_JoystickClose(gui_joystick);
  }
  if (gui_pixel_format != NULL) {
    SDL_FreeFormat(gui_pixel_format);
  }
  if (gui_texture != NULL) {
    SDL_UnlockTexture(gui_texture);
    SDL_DestroyTexture(gui_texture);
  }
  if (gui_renderer != NULL) {
    SDL_DestroyRenderer(gui_renderer);
  }
  if (gui_window != NULL) {
    SDL_DestroyWindow(gui_window);
  }
  SDL_Quit();
}



int gui_init(int joystick_no, bool disable_audio)
{
#ifdef DISABLE_GUI
  if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) != 0) {
#else
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) != 0) {
#endif /* DISABLE_GUI */
    fprintf(stderr, "Unable to initalize SDL: %s\n", SDL_GetError());
    return -1;
  }
  atexit(gui_exit_handler);

#ifndef DISABLE_GUI
  if ((gui_window = SDL_CreateWindow("lazyboNES",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    GUI_WIDTH, GUI_HEIGHT, 0)) == NULL) {
    fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
    return -1;
  }

  if ((gui_renderer = SDL_CreateRenderer(gui_window, -1, 0)) == NULL) {
    fprintf(stderr, "Unable to create renderer: %s\n", SDL_GetError());
    return -1;
  }

  if ((gui_texture = SDL_CreateTexture(gui_renderer, 
    SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
    GUI_WIDTH, GUI_HEIGHT)) == NULL) {
    fprintf(stderr, "Unable to create texture: %s\n", SDL_GetError());
    return -1;
  }

  if (SDL_LockTexture(gui_texture, NULL,
    (void **)&gui_pixels, &gui_pixel_pitch) != 0) {
    fprintf(stderr, "Unable to lock texture: %s\n", SDL_GetError());
    return -1;
  }

  if ((gui_pixel_format = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888)) == NULL) {
    fprintf(stderr, "Unable to create pixel format: %s\n", SDL_GetError());
    return -1;
  }
#endif /* DISABLE_GUI */

  if (SDL_NumJoysticks() > joystick_no) {
    gui_joystick = SDL_JoystickOpen(joystick_no);
    fprintf(stderr, "Found Joystick: %s\n", SDL_JoystickName(gui_joystick));
  }

  if (disable_audio) {
    return 0;
  } else {
    return gui_audio_init();
  }
}



void gui_draw_scanline(uint16_t y, uint8_t colors[])
{
#ifndef DISABLE_GUI
  int x;

  for (x = 0; x < GUI_WIDTH; x++) {
    gui_pixels[(y * GUI_WIDTH) + x] = SDL_MapRGB(gui_pixel_format, 
      gui_sys_palette[colors[x] % 64][0],
      gui_sys_palette[colors[x] % 64][1],
      gui_sys_palette[colors[x] % 64][2]);
  }
#endif /* DISABLE_GUI */
}



uint8_t gui_get_controller_state(void)
{
  return gui_controller_state;
}



bool gui_save_state_requested(void)
{
  if (gui_save_state_request) {
    gui_save_state_request = false;
    return true;
  } else {
    return false;
  }
}



bool gui_load_state_requested(void)
{
  if (gui_load_state_request) {
    gui_load_state_request = false;
    return true;
  } else {
    return false;
  }
}



void gui_update(void)
{
  SDL_Event event;

  while (SDL_PollEvent(&event) == 1) {
    switch (event.type) {
    case SDL_QUIT:
      exit(0);
      break;

    /* Keyboard-based Controller */
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      switch (event.key.keysym.sym) {
      case SDLK_SPACE:
      case SDLK_z: /* A */
        if (event.type == SDL_KEYDOWN) {
          gui_controller_state |= 0x1;
        } else {
          gui_controller_state &= ~0x1;
        }
        break;

      case SDLK_x: /* B */
        if (event.type == SDL_KEYDOWN) {
          gui_controller_state |= 0x2;
        } else {
          gui_controller_state &= ~0x2;
        }
        break;

      case SDLK_c: /* Select */
        if (event.type == SDL_KEYDOWN) {
          gui_controller_state |= 0x4;
        } else {
          gui_controller_state &= ~0x4;
        }
        break;

      case SDLK_RETURN:
      case SDLK_v: /* Start */
        if (event.type == SDL_KEYDOWN) {
          gui_controller_state |= 0x8;
        } else {
          gui_controller_state &= ~0x8;
        }
        break;

      case SDLK_UP:
        if (event.type == SDL_KEYDOWN) {
          gui_controller_state |= 0x10;
        } else {
          gui_controller_state &= ~0x10;
        }
        break;

      case SDLK_DOWN:
        if (event.type == SDL_KEYDOWN) {
          gui_controller_state |= 0x20;
        } else {
          gui_controller_state &= ~0x20;
        }
        break;

      case SDLK_LEFT:
        if (event.type == SDL_KEYDOWN) {
          gui_controller_state |= 0x40;
        } else {
          gui_controller_state &= ~0x40;
        }
        break;

      case SDLK_RIGHT:
        if (event.type == SDL_KEYDOWN) {
          gui_controller_state |= 0x80;
        } else {
          gui_controller_state &= ~0x80;
        }
        break;

      case SDLK_F5: /* Save State */
        if (event.type == SDL_KEYDOWN) {
          gui_save_state_request = true;
        }
        break;

      case SDLK_F8: /* Load State */
        if (event.type == SDL_KEYDOWN) {
          gui_load_state_request = true;
        }
        break;

      case SDLK_q: /* Quit */
        if (event.type == SDL_KEYDOWN) {
          exit(0);
        }
        break;
      }
      break;

    /* Joystick-based Controller */
    case SDL_JOYAXISMOTION:
      if (event.jaxis.axis == 0) {
        if (event.jaxis.value > 16384) { /* Right Pressed */
          gui_controller_state |=  0x80;
          gui_controller_state &= ~0x40;
        } else if (event.jaxis.value < -16384) { /* Left Pressed */
          gui_controller_state &= ~0x80;
          gui_controller_state |=  0x40;
        } else {
          gui_controller_state &= ~0xC0; /* Right/Left Released */
        }

      } else if (event.jaxis.axis == 1) {
        if (event.jaxis.value > 16384) { /* Down Pressed */
          gui_controller_state |=  0x20;
          gui_controller_state &= ~0x10;
        } else if (event.jaxis.value < -16384) { /* Up Pressed */
          gui_controller_state &= ~0x20;
          gui_controller_state |=  0x10;
        } else {
          gui_controller_state &= ~0x30; /* Down/Up Released */
        }
      }
      break;

    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
      switch (event.jbutton.button) {
      case 1: /* A */
        if (event.jbutton.state == 1) {
          gui_controller_state |= 0x1;
        } else {
          gui_controller_state &= ~0x1;
        }
        break;

      case 0: /* B */
      case 2:
      case 3:
        if (event.jbutton.state == 1) {
          gui_controller_state |= 0x2;
        } else {
          gui_controller_state &= ~0x2;
        }
        break;

      case 4: /* Save State */
        if (event.jbutton.state == 1) {
          gui_save_state_request = true;
        }
        break;

      case 5: /* Load State */
        if (event.jbutton.state == 1) {
          gui_load_state_request = true;
        }
        break;

      case 6: /* Select */
        if (event.jbutton.state == 1) {
          gui_controller_state |= 0x4;
        } else {
          gui_controller_state &= ~0x4;
        }
        break;

      case 7: /* Start */
        if (event.jbutton.state == 1) {
          gui_controller_state |= 0x8;
        } else {
          gui_controller_state &= ~0x8;
        }
        break;
      }
      break;
    }
  }

  if (gui_renderer != NULL) {
    SDL_UnlockTexture(gui_texture);

    SDL_RenderCopy(gui_renderer, gui_texture, NULL, NULL);

    if (SDL_LockTexture(gui_texture, NULL,
      (void **)&gui_pixels, &gui_pixel_pitch) != 0) {
      fprintf(stderr, "Unable to lock texture: %s\n", SDL_GetError());
      exit(0);
    }
  }

  /* Force 60 Hz (NTSC) */
  while ((SDL_GetTicks() - gui_ticks) < 16) {
    SDL_Delay(1);
  }

  if (gui_renderer != NULL) {
    SDL_RenderPresent(gui_renderer);
  }

  gui_ticks = SDL_GetTicks();
}



