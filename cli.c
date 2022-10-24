#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <curses.h>
#include <signal.h>

#ifdef EXTRA_INFO
#include "mem.h"
#include "ppu.h"
#include "apu.h"
#endif



#define CLI_WIDTH 32
#define CLI_HEIGHT 30

#define CLI_BUTTON_TIME_SET 22



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
  '|',' ',' ','|','\\','\\','|','.','v','Y','Y','Y',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
}};

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



static bool cli_enable_colors = false;
static uint8_t cli_controller_state = 0;
static uint16_t cli_button_timeout[8] = {0,0,0,0,0,0,0,0};
static const uint8_t cli_button_map[8] = 
  {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

static bool cli_freeze = false;
static bool cli_freeze_step = false;



static void cli_exit_handler(void)
{
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
  endwin();
  timeout(-1);
}



void cli_resume(void)
{
  timeout(0);
  refresh();
}



int cli_init(bool enable_colors)
{
  cli_enable_colors = enable_colors;
#ifndef DISABLE_CLI
  int maxy, maxx;

  initscr();

  getmaxyx(stdscr, maxy, maxx);
  if (maxx < CLI_WIDTH || maxy < CLI_HEIGHT) {
    endwin();
    fprintf(stderr, "\nConsole window must at least %dx%d.\n",
      CLI_WIDTH, CLI_HEIGHT);
    return -1;
  }

  atexit(cli_exit_handler);
  noecho();
  keypad(stdscr, TRUE);
  timeout(0); /* Non-blocking mode. */

  if (cli_enable_colors && has_colors()) {
    start_color();
    init_pair(1, COLOR_RED,     COLOR_BLACK);
    init_pair(2, COLOR_GREEN,   COLOR_BLACK);
    init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(4, COLOR_BLUE,    COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN,    COLOR_BLACK);
    init_pair(7, COLOR_WHITE,   COLOR_BLACK);
  }

#endif /* DISABLE_CLI */
  return 0;
}



void cli_draw_tile(uint8_t y, uint8_t x, bool table_no, uint8_t tile)
{
  if (x >= CLI_WIDTH) {
    return;
  }
  if (y >= CLI_HEIGHT) {
    return;
  }

  chtype ch = cli_tile_map[table_no][tile];
  int color_pair = cli_color_map[table_no][tile];

  if (! (table_no == 0 && ch == ' ')) {
    if (cli_enable_colors && has_colors()) {
      attron(COLOR_PAIR(color_pair));
    }
    mvaddch(y, x, ch);
    if (cli_enable_colors && has_colors()) {
      attroff(COLOR_PAIR(color_pair));
    }
  }
}



uint8_t cli_get_controller_state(void)
{
  return cli_controller_state;
}



#ifdef EXTRA_INFO
void cli_update(mem_t *mem, ppu_t *ppu, apu_t* apu)
#else
void cli_update(void)
#endif
{
#ifndef DISABLE_CLI
  int c, i;

  for (i = 0; i < 8; i++) {
    if (cli_button_timeout[i] > 0) {
      cli_button_timeout[i]--;
    } else {
      cli_controller_state &= ~cli_button_map[i];
    }
  }

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

  while ((c = getch()) != ERR) {
    switch (c) {
    case KEY_RESIZE:
      /* Use this event instead of SIGWINCH for better portability. */
      cli_winch_handler();
      break;

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

    case 'Q':
    case 'q':
      exit(0);
      break;

    default:
      break;
    }
  }

  if (cli_freeze) {
    goto cli_update_freeze;
  }
#endif /* DISABLE_CLI */
}



