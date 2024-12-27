#include "cli.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <curses.h>
#include <signal.h>
#include <ctype.h>

#include "kbd.h"
#ifdef EXTRA_INFO
#include "mem.h"
#include "ppu.h"
#include "apu.h"
#endif



#define CLI_WIDTH 32
#define CLI_HEIGHT 30

#define CLI_BUTTON_TIME_SET 22
#define CLI_TEXT_INJECT_DELAY_NORMAL 4
#define CLI_TEXT_INJECT_DELAY_RETURN 32



/* ASCII character to use for each tile: */
static const chtype cli_tile_map[2][UINT8_MAX + 1] = {
{
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '0','*','*','*','+','+','+','+','@','@','@','=','@','@','@','@',
  '$','$','$','$','*','*','*','*','x','k','k','k','k','k','k','k',
  'g','g','g','g','o','O','m','m','m','m','x','x','h','h','F','F',
  'h','h','h','h','x','x','x','x','h','h','h','h','h','S','s','s',
  '@','@','@','@','s','s','s','s','s','s','s','s','s','s','@','@',
  'k','k','k','k','k','k','k','k','k','k','b','b','b','b','b','b',
  'b','b','c','c','c','c','c','c','l','l','l','l','l','l','B','B',
  'B','B','B','B','B','B','B','B','B','B','B','B','B','t','t','t',
  '*','h','h','h','h','h','f','*','x','f','x','x','j','j','j','j',
  'Y','Y','h','h','S','p','p','d','d','d','d','p','p','p','p','x',
  't','t','t','t','b','b','1','2','4','5','8','0',' ','U','P',' ',
},{
  '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
  'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
  'W','X','Y','Z',' ',' ',' ',' ','-','x','T','!','=','=','$',' ',
  '/','/','\\','\\','.','/','/','\\','\\','\\','-','-','/',' ','-','O',
  'T','w','|','|',' ','%',' ','%',' ',' ',' ','#','#','#','#','#',
  '#','#',':','?','?','?','?','#','#','#','#','-','O','#','#',' ',
  '#','#','#','#','#','#','#','#','#','#','#','=','=','=','=','=',
  '=','=','=','=','=','|','|','=',' ','=',' ','A','A','A','A','/',
  '-','-','#','#','#','#','#','#','#','#','#','#','#','#',' ',' ',
  ' ','#','#','#','#',' ',' ',' ',' ','-','|',' ',' ','%','%',' ',
  '-','-','|','|','Y','$','$','$','$','%','%','#','#','#','#','.',
  '/','\\','\\','/','#','#','#','#','/','\\','|','|','\\','/','|','|',
  ' ','=','$','$','$','$','T','T','T','T','T','T','T','T','*','c',
  '/','\\','\\','\\','\\','/',' ',' ','|','/','|','|','-',' ','|','<',
  '|',' ',' ','|','\\','\\','|','.','v','Y','Y','Y',' ','/','/',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
}};

/* ASCII character to use for each tile in BASIC mode: */
static const chtype cli_tile_map_basic[2][UINT8_MAX + 1] = {
{
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
},{
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  ' ','!','"','#','$','%','&','\'','(',')','*','+',',','-','.','/',
  '0','1','2','3','4','5','6','7','8','9',':',';','<','=','>','?',
  '@','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
  'P','Q','R','S','T','U','V','W','X','Y','Z','[','\\',']','^','_',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@',
  '@','@','@','@','@','@','@','@','@','@','@','@','@','#','@','@',
}};

/* Color to use for tile in 8/16 color mode: */
static const int cli_color_map[2][UINT8_MAX + 1] = {
{
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  7, 1, 1, 1, 7, 7, 7, 7, 1, 1, 1, 7, 1, 1, 1, 1,
  3, 3, 3, 3, 1, 1, 1, 1, 7, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 7, 7, 1, 1, 1, 1, 7, 7, 5, 5, 4, 4,
  5, 5, 5, 5, 7, 7, 7, 7, 5, 5, 5, 5, 5, 3, 5, 5,
  1, 1, 1, 1, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 1, 1,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 7, 7,
  1, 5, 5, 5, 5, 5, 1, 1, 7, 1, 7, 7, 6, 6, 6, 6,
  2, 2, 5, 5, 3, 2, 2, 4, 4, 4, 4, 2, 2, 2, 2, 7,
  7, 7, 7, 7, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
},{
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7,
  2, 2, 2, 2, 2, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 3, 3, 3, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 2, 2, 2, 2, 2, 2, 2, 2, 7, 7,
  7, 2, 2, 2, 2, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 3, 3, 3, 3, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 3, 3, 3, 3, 7, 7, 7, 7, 7, 7, 7, 7, 1, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
}};

/* Color mapping in 256 color mode, from the default rxvt color palette: */
static const uint8_t cli_sys_palette[64] =
{
  242,  20,  21,  56,  54,  88,  52,  52,  22,  22,  22,  22,  18,  16, 16, 16,
  246,  26,  27,  98, 128, 162, 130,  94,  64,  28,  28,  28,  25,  16, 16, 16,
  255,  33,  63, 134, 206, 204, 202, 208, 112, 118, 118,  84, 255, 240, 16, 16,
  255, 117, 105, 182, 212, 218, 217, 215, 185, 154, 156, 120, 255, 250, 16, 16,
};

/* Foreground color number to use for tile in 256 color mode: */
static const int cli_fg_palette_map[2][UINT8_MAX + 1] = {
{
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
},{
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
}};

/* Background color number to use for background tile in 256 color mode: */
static const int cli_bg_palette_map[UINT8_MAX + 1] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 3, 0, 3, 0, 0, 0, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 3, 3, 0,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 0, 0, 3, 0, 3, 0, 0, 0, 0, 0, 0,
  0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0,
  0, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 0,
  0, 0, 0, 0, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};



static bool cli_active = false;
static bool cli_enable_colors = false;
static bool cli_basic_mode = false;
static FILE *cli_text_inject_fh = NULL;
static int cli_maxx;
static int cli_maxy;
static uint8_t cli_controller_state = 0;
#ifndef SPECIAL_TERMINAL
static uint16_t cli_button_timeout[8] = {0,0,0,0,0,0,0,0};
static const uint8_t cli_button_map[8] = 
  {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
#endif

static bool cli_freeze = false;
static bool cli_freeze_step = false;



static void cli_exit_handler(void)
{
#ifdef SPECIAL_TERMINAL
  fprintf(stderr, "\e[x"); /* Silence */
#endif
  endwin();
}



static void cli_winch_handler(void)
{
  refresh();
  flushinp();
  keypad(stdscr, TRUE);
}



void cli_pause(void)
{
  if (! cli_active) {
    return;
  }

  endwin();
  timeout(-1);
}



void cli_resume(void)
{
  if (! cli_active) {
    return;
  }

  timeout(0);
  refresh();
}



int cli_init(bool enable_colors, bool basic_mode)
{
  int fg;
  int bg;

  cli_active = true;
  cli_enable_colors = enable_colors;
  if (basic_mode) {
    cli_basic_mode = true;
    cli_enable_colors = false; /* Override and disable colors! */
  }

  initscr();
  getmaxyx(stdscr, cli_maxy, cli_maxx);
  atexit(cli_exit_handler);
  noecho();
  keypad(stdscr, TRUE);
  timeout(0); /* Non-blocking mode. */

  if (cli_enable_colors && has_colors()) {
    start_color();

    if (COLOR_PAIRS >= 4096) {
      /* 256 Color Mode */
      use_default_colors();
      for (bg = 0; bg < 64; bg++) {
        for (fg = 0; fg < 64; fg++) {
          init_pair((bg * 64) + fg + 1,
            cli_sys_palette[fg], cli_sys_palette[bg]);
        }
      }
    } else {
      /* 8 Color Mode */
      init_pair(1, COLOR_RED,     COLOR_BLACK);
      init_pair(2, COLOR_GREEN,   COLOR_BLACK);
      init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
      init_pair(4, COLOR_BLUE,    COLOR_BLACK);
      init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
      init_pair(6, COLOR_CYAN,    COLOR_BLACK);
      init_pair(7, COLOR_WHITE,   COLOR_BLACK);
    }
  }

  return 0;
}



void cli_draw_tile(uint8_t y, uint8_t x, bool table_no, uint8_t tile,
  uint8_t color_backdrop,
  uint8_t color_1,
  uint8_t color_2,
  uint8_t color_3,
  uint8_t color_4)
{
  chtype ch;

  if (! cli_active) {
    return;
  }
  if (x >= CLI_WIDTH) {
    return;
  }
  if (y >= CLI_HEIGHT) {
    return;
  }

  /* Crop from top if screen height is reduced: */
  if (cli_maxy < 25) {
    y -= 3;
  } else if (cli_maxy < 26) {
    y -= 2;
  } else if (cli_maxy < 27) {
    y -= 1;
  }

  if (cli_basic_mode) {
    ch = cli_tile_map_basic[table_no][tile];
  } else {
    ch = cli_tile_map[table_no][tile];
  }

  if ((table_no == 0 && ch == ' ')) {
    return; /* Do not draw transparent part of sprites. */
  }

  if (COLOR_PAIRS >= 4096) {
    /* 256 Color Mode */
    int fg;
    int bg;

    switch (cli_fg_palette_map[table_no][tile]) {
    case 1:
      fg = color_1;
      break;
    case 2:
      fg = color_2;
      break;
    case 3:
      fg = color_3;
      break;
    case 4:
      fg = color_4;
      break;
    case 0:
    default:
      fg = color_backdrop;
      break;
    }

    if (table_no == 0) {
      bg = color_backdrop;
    } else {
      switch (cli_bg_palette_map[tile]) {
      case 1:
        bg = color_1;
        break;
      case 2:
        bg = color_2;
        break;
      case 3:
        bg = color_3;
        break;
      case 4:
        bg = color_4;
        break;
      case 0:
      default:
        bg = color_backdrop;
        break;
      }
    }

    short color_pair = (bg * 64) + fg + 1;
    attr_set(table_no == 0 ? A_BOLD : A_NORMAL, color_pair, NULL);
    mvaddch(y, x, ch);
    attr_set(A_NORMAL, 0, NULL);

  } else {
    /* 8/16 Color Mode or Monochrome */
    int color_pair = cli_color_map[table_no][tile];

    if (table_no == 0) {
      attron(A_BOLD);
    }
    if (cli_enable_colors && has_colors()) {
      attron(COLOR_PAIR(color_pair));
    }
    mvaddch(y, x, ch);
    if (cli_enable_colors && has_colors()) {
      attroff(COLOR_PAIR(color_pair));
    }
    if (table_no == 0) {
      attroff(A_BOLD);
    }
  }
}



uint8_t cli_get_controller_state(void)
{
  return cli_controller_state;
}



static bool cli_key_is_shifted(int key)
{
  if (key == '!' ||
      key == '"' ||
      key == '#' ||
      key == '$' ||
      key == '%' ||
      key == '&' ||
      key == '(' ||
      key == ')' ||
      key == '<' ||
      key == '>' ||
      key == '+' ||
      key == '=' ||
      key == '*' ||
      key == '?' ||
      key == '\'' ||
      key == '_') {
    return true;
  }

  return false;
}



static bool cli_key_is_control(int key)
{
  if (key == '\r' || key == '\n') {
    return false;
  } else if (key < 0x20) {
    return true;
  } else {
    return false;
  }
}



int cli_text_inject(const char *filename)
{
  if (cli_text_inject_fh != NULL) {
    return -2; /* Inject already in progress. */
  }

  cli_text_inject_fh = fopen(filename, "rb");
  if (cli_text_inject_fh == NULL) {
    return -1; /* File not found. */
  }

  return 0;
}



static kbd_key_t cli_key_map(int key)
{
  switch (key) {
  case '1':
  case '!':
    return KBD_KEY_1;
  case '2':
  case '"':
    return KBD_KEY_2;
  case '3':
  case '#':
    return KBD_KEY_3;
  case '4':
  case '$':
    return KBD_KEY_4;
  case '5':
  case '%':
    return KBD_KEY_5;
  case '6':
  case '&':
    return KBD_KEY_6;
  case '7':
  case '\'':
    return KBD_KEY_7;
  case '8':
  case '(':
    return KBD_KEY_8;
  case '9':
  case ')':
    return KBD_KEY_9;
  case '0':
    return KBD_KEY_0;

  case 'a':
  case 'A':
  case 0x01:
    return KBD_KEY_A;
  case 'b':
  case 'B':
    return KBD_KEY_B;
  case 'c':
  case 'C':
  case 0x03:
    return KBD_KEY_C;
  case 'd':
  case 'D':
    return KBD_KEY_D;
  case 'e':
  case 'E':
  case 0x05:
    return KBD_KEY_E;
  case 'f':
  case 'F':
    return KBD_KEY_F;
  case 'g':
  case 'G':
  case 0x07:
    return KBD_KEY_G;
  case 'h':
  case 'H':
  case 0x08:
    return KBD_KEY_H;
  case 'i':
  case 'I':
  case '\t':
    return KBD_KEY_I;
  case 'j':
  case 'J':
    return KBD_KEY_J;
  case 'k':
  case 'K':
  case 0x0B:
    return KBD_KEY_K;
  case 'l':
  case 'L':
  case 0x0C:
    return KBD_KEY_L;
  case 'm':
  case 'M':
    return KBD_KEY_M;
  case 'n':
  case 'N':
  case 0x0E:
    return KBD_KEY_N;
  case 'o':
  case 'O':
  case 0x0F:
    return KBD_KEY_O;
  case 'p':
  case 'P':
  case 0x10:
    return KBD_KEY_P;
  case 'q':
  case 'Q':
  case 0x11:
    return KBD_KEY_Q;
  case 'r':
  case 'R':
  case 0x12:
    return KBD_KEY_R;
  case 's':
  case 'S':
  case 0x13:
    return KBD_KEY_S;
  case 't':
  case 'T':
  case 0x14:
    return KBD_KEY_T;
  case 'u':
  case 'U':
  case 0x15:
    return KBD_KEY_U;
  case 'v':
  case 'V':
  case 0x16:
    return KBD_KEY_V;
  case 'w':
  case 'W':
  case 0x17:
    return KBD_KEY_W;
  case 'x':
  case 'X':
  case 0x18:
    return KBD_KEY_X;
  case 'y':
  case 'Y':
    return KBD_KEY_Y;
  case 'z':
  case 'Z':
    return KBD_KEY_Z;

  case KEY_F(1):
    return KBD_KEY_F1;
  case KEY_F(2):
    return KBD_KEY_F2;
  case KEY_F(3):
    return KBD_KEY_F3;
  case KEY_F(4):
    return KBD_KEY_F4;
  case KEY_F(5):
    return KBD_KEY_F5;
  case KEY_F(6):
    return KBD_KEY_F6;
  case KEY_F(7):
    return KBD_KEY_F7;
  case KEY_F(8):
    return KBD_KEY_F8;
  case KEY_F(9):
    return KBD_KEY_STOP;
  case KEY_F(10):
    return KBD_KEY_KANA;
  case KEY_F(11):
    return KBD_KEY_GRPH;

  case '-':
  case '=':
    return KBD_KEY_MINUS;
  case '^':
    return KBD_KEY_CARET;
  case '\\':
    return KBD_KEY_YEN;
  case '\e':
    return KBD_KEY_ESC;
  case '@':
    return KBD_KEY_AT;
  case '[':
    return KBD_KEY_LEFT_BRACKET;
  case ']':
    return KBD_KEY_RIGHT_BRACKET;
  case ';':
  case '+':
    return KBD_KEY_SEMICOLON;
  case ':':
  case '*':
    return KBD_KEY_COLON;
  case ',':
  case '<':
    return KBD_KEY_COMMA;
  case '.':
  case '>':
    return KBD_KEY_PERIOD;
  case '/':
  case '?':
    return KBD_KEY_SLASH;
  case '_':
    return KBD_KEY_UNDERSCORE;

  case ' ':
    return KBD_KEY_SPACE;
  case '\r':
  case '\n':
  case KEY_ENTER:
    return KBD_KEY_RETURN;
  case KEY_BACKSPACE:
    return KBD_KEY_DEL;
  case KEY_HOME:
    return KBD_KEY_CLR_HOME;
  case KEY_IC:
    return KBD_KEY_INS;

  case KEY_UP:
    return KBD_KEY_UP;
  case KEY_DOWN:
    return KBD_KEY_DOWN;
  case KEY_LEFT:
    return KBD_KEY_LEFT;
  case KEY_RIGHT:
    return KBD_KEY_RIGHT;

  case ERR:
  default:
    return KBD_KEY_NONE;
  }
}



#ifdef SPECIAL_TERMINAL
void cli_audio_update(uint16_t freq, uint8_t volume)
{
  /* Send special custom CSI codes with stderr to bypass curses. */
  if (volume > 0) {
    fprintf(stderr, "\e[%dx", freq);
  } else {
    fprintf(stderr, "\e[x"); /* Silence */
  }
}
#endif



#ifdef EXTRA_INFO
void cli_update(mem_t *mem, ppu_t *ppu, apu_t* apu)
#else
void cli_update(void)
#endif
{
  int c;

  if (! cli_active) {
    return;
  }

#ifndef SPECIAL_TERMINAL
  for (int i = 0; i < 8; i++) {
    if (cli_button_timeout[i] > 0) {
      cli_button_timeout[i]--;
    } else {
      cli_controller_state &= ~cli_button_map[i];
    }
  }
#endif

#ifdef EXTRA_INFO
  static uint32_t frame_start = 0;
  static uint32_t frame_end = 0;
  static bool axe84 = false;
  int minutes;
  double seconds;

  /* Check for 400 to appear under TIME. */
  if (frame_start == 0 &&
      ppu->name_table[0x7A] == 0x04) { /* '4' Symbol */
    frame_start = ppu->frame_no;
  }

  /* Check for the axe at world 8-4. */
  if (axe84 == false &&
      mem->ram[0x75F] == 7 && /* World 8 */
      ppu->name_table[0x21A] == 0x7B) { /* Top-Left Axe */
    axe84 = true;
  }

  /* Check if the axe is gone. */
  if (frame_end == 0 &&
      axe84 == true &&
      ppu->name_table[0x21A] == 0x24) { /* Empty Space */
    frame_end = ppu->frame_no;
  }

  mvprintw(3, 40, "Frame      : %d", ppu->frame_no);
  mvprintw(4, 40, "Sub-Pixel X: 0x%01x", mem->ram[0x400] / 16);
  mvprintw(5, 40, "Controller : %c%c%c%c%c%c%c%c",
    apu->controller[0].data.a      ? 'A' : '.',
    apu->controller[0].data.b      ? 'B' : '.',
    apu->controller[0].data.select ? 'S' : '.',
    apu->controller[0].data.start  ? 'T' : '.',
    apu->controller[0].data.up     ? 'U' : '.',
    apu->controller[0].data.down   ? 'D' : '.',
    apu->controller[0].data.left   ? 'L' : '.',
    apu->controller[0].data.right  ? 'R' : '.');
  if (frame_start == 0) {
    mvprintw(6, 40, "Time       : -");
  } else {
    if (frame_end == 0) {
      seconds = (ppu->frame_no - frame_start) / 60.0988;
    } else {
      seconds = (frame_end - frame_start) / 60.0988;
    }
    minutes = (int)seconds / 60;
    mvprintw(6, 40, "Time       : %02d:%06.3f",
      minutes, seconds - (minutes * 60));
  }
#endif

  if (cli_freeze_step) {
    cli_freeze = true;
    cli_freeze_step = false;
  }
cli_update_freeze:
  refresh();
  getmaxyx(stdscr, cli_maxy, cli_maxx);

  if (cli_basic_mode) {
    if (cli_text_inject_fh != NULL) {
      static int inject_delay = 0;

      if (inject_delay > 0) {
        inject_delay--;
      } else {
        c = fgetc(cli_text_inject_fh);
        if (c == EOF) {
          fclose(cli_text_inject_fh);
          cli_text_inject_fh = NULL;
        }
        kbd_key_set(cli_key_map(c),
                    cli_key_is_shifted(c),
                    cli_key_is_control(c));
        if (cli_key_map(c) == KBD_KEY_RETURN) {
          inject_delay = CLI_TEXT_INJECT_DELAY_RETURN;
        } else {
          inject_delay = CLI_TEXT_INJECT_DELAY_NORMAL;
        }
      }
      return;
    }

    while ((c = getch()) != ERR) {
      if (c == KEY_RESIZE) {
        cli_winch_handler();
      } else {
        kbd_key_set(cli_key_map(c),
                    cli_key_is_shifted(c),
                    cli_key_is_control(c));
      }
    }

  } else {
    while ((c = getch()) != ERR) {
      switch (c) {
      case KEY_RESIZE:
        /* Use this event instead of SIGWINCH for better portability. */
        cli_winch_handler();
        break;

#ifdef SPECIAL_TERMINAL /* Separate key press and key release events. */
      case 'h': /* Press A */
      case ',':
        cli_controller_state |= 0x1;
        break;

      case 'j': /* Press B */
      case '0':
        cli_controller_state |= 0x2;
        break;

      case 'k': /* Press Select */
        cli_controller_state |= 0x4;
        break;

      case 'l': /* Press Start */
        cli_controller_state |= 0x8;
        break;

      case 'w': /* Press Up */
        cli_controller_state |= 0x10;
        break;

      case 's': /* Press Down */
        cli_controller_state |= 0x20;
        break;

      case 'a': /* Press Left */
        cli_controller_state |= 0x40;
        break;

      case 'd': /* Press Right */
        cli_controller_state |= 0x80;
        break;

      case 'H': /* Release A */
      case ';':
        cli_controller_state &= ~0x1;
        break;

      case 'J': /* Release B */
      case '=':
        cli_controller_state &= ~0x2;
        break;

      case 'K': /* Release Select */
        cli_controller_state &= ~0x4;
        break;

      case 'L': /* Release Start */
        cli_controller_state &= ~0x8;
        break;

      case 'W': /* Release Up */
        cli_controller_state &= ~0x10;
        break;

      case 'S': /* Release Down */
        cli_controller_state &= ~0x20;
        break;

      case 'A': /* Release Left */
        cli_controller_state &= ~0x40;
        break;

      case 'D': /* Release Right */
        cli_controller_state &= ~0x80;
        break;

#else
      case ' ':
      case 'z': /* A */
        cli_controller_state |= 0x1;
        cli_button_timeout[0] = CLI_BUTTON_TIME_SET;
        break;

      case 'x': /* B */
        cli_controller_state |= 0x2;
        cli_button_timeout[1] = CLI_BUTTON_TIME_SET;
        break;

      case 'c': /* Select */
        cli_controller_state |= 0x4;
        cli_button_timeout[2] = CLI_BUTTON_TIME_SET;
        break;

      case KEY_ENTER:
      case '\n':
      case '\r':
      case 'v': /* Start */
        cli_controller_state |= 0x8;
        cli_button_timeout[3] = CLI_BUTTON_TIME_SET;
        break;

      case KEY_UP:
        cli_controller_state |= 0x10;
        cli_button_timeout[4] = CLI_BUTTON_TIME_SET;
        break;

      case KEY_DOWN:
        cli_controller_state |= 0x20;
        cli_button_timeout[5] = CLI_BUTTON_TIME_SET;
        break;

      case KEY_LEFT:
        cli_controller_state |= 0x40;
        cli_button_timeout[6] = CLI_BUTTON_TIME_SET;
        break;

      case KEY_RIGHT:
        cli_controller_state |= 0x80;
        cli_button_timeout[7] = CLI_BUTTON_TIME_SET;
        break;

      case 'P':
      case 'p':
        cli_freeze = !cli_freeze;
        break;

      case '.':
        cli_freeze = false;
        cli_freeze_step = true;
        break;
#endif

      case 'Q':
      case 'q':
        exit(EXIT_SUCCESS);
        break;

      default:
        break;
      }
    }

    if (cli_freeze) {
      goto cli_update_freeze;
    }
  }
}



