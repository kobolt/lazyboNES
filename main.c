#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "cpu.h"
#include "mem.h"
#include "panic.h"
#include "ines.h"
#include "ppu.h"
#include "apu.h"
#include "fds.h"
#include "kbd.h"
#include "gui.h"
#include "cli.h"
#include "tas.h"



#define DEBUGGER_KEYBOARD_INJECT_FILE "input.txt"
#define DEBUGGER_CASSETTE_LOAD_FILE "cas.wav"
#define DEBUGGER_CASSETTE_SAVE_FILE "cas.wav"



static cpu_t main_cpu;
static mem_t main_mem;
static ppu_t main_ppu;
static apu_t main_apu;
static fds_t main_fds;

static bool debugger_break = false;
static bool nmi_break = false;

static bool saved_state = false;
static cpu_t save_cpu;
static mem_t save_mem;
static ppu_t save_ppu;
static apu_t save_apu;
static fds_t save_fds;



static bool debugger(void)
{
  int i;
  char cmd[16];
  int result;

  fprintf(stdout, "\n");
  while (1) {
    fprintf(stdout, "%08d:%04x> ", main_ppu.frame_no, main_cpu.pc);

    if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
      if (feof(stdin)) {
        exit(EXIT_SUCCESS);
      }
      continue;
    }

    switch (cmd[0]) {
    case '?':
    case 'h':
      fprintf(stdout, "Commands:\n");
      fprintf(stdout, "  q - Quit\n");
      fprintf(stdout, "  h - Help\n");
      fprintf(stdout, "  c - Continue\n");
      fprintf(stdout, "  n - Continue until next NMI\n");
      fprintf(stdout, "  s - Step\n");
      fprintf(stdout, "  w - Warp mode toggle\n");
      fprintf(stdout, "  1 - Dump CPU Trace\n");
      fprintf(stdout, "  2 - Dump ZP/Stack/Vectors\n");
      fprintf(stdout, "  3 - Dump PPU NT/AT/RAM\n");
      fprintf(stdout, "  4 - Dump PPU Pattern Tables\n");
      fprintf(stdout, "  5 - Dump APU\n");
      fprintf(stdout, "  6 - Dump other RAM\n");
      fprintf(stdout, "  7 - Dump FDS\n");
      fprintf(stdout, "BASIC Mode Commands:\n");
      fprintf(stdout, "  t - Inject \""
        DEBUGGER_KEYBOARD_INJECT_FILE "\" text file as keyboard input.\n");
      fprintf(stdout, "  l - Load \""
        DEBUGGER_CASSETTE_LOAD_FILE "\" cassette WAV file.\n");
      fprintf(stdout, "  k - Start saving \""
        DEBUGGER_CASSETTE_SAVE_FILE "\" cassette WAV file.\n");
      fprintf(stdout, "  K - Stop saving cassette WAV file.\n");
      break;

    case 'c': /* Continue */
      return false;

    case 's': /* Step */
      return true;

    case 'n': /* Continue until next NMI. */
      nmi_break = true;
      return false;

    case 'w':
      if (gui_warp_mode_get()) {
        gui_warp_mode_set(false);
        fprintf(stdout, "Warp mode disabled.\n");
      } else {
        gui_warp_mode_set(true);
        fprintf(stdout, "Warp mode enabled.\n");
      }
      break;

    case 't':
      result = cli_text_inject(DEBUGGER_KEYBOARD_INJECT_FILE);
      if (result != 0) {
        fprintf(stdout, "Failed to inject text file! Error Code: %d\n",
            result);
      } else {
        fprintf(stdout, "Injecting text file...\n");
      }
      break;

    case 'l':
      result = kbd_cassette_load_file(DEBUGGER_CASSETTE_LOAD_FILE);
      if (result != 0) {
        fprintf(stdout, "Failed to start load WAV file! Code: %d\n",
          result);
      } else {
        fprintf(stdout, "Cassette WAV file playback...\n");
      }
      break;

    case 'k':
      result = kbd_cassette_save_file_start(DEBUGGER_CASSETTE_SAVE_FILE);
      if (result != 0) {
        fprintf(stdout, "Failed to start saving of WAV file! Code: %d\n",
          result);
      } else {
        fprintf(stdout, "Cassette WAV file recording...\n");
      }
      break;

    case 'K':
      result = kbd_cassette_save_file_stop();
      if (result != 0) {
        fprintf(stdout, "Failed to stop saving of WAV file! Code: %d\n",
          result);
      } else {
        fprintf(stdout, "Cassette WAV file recording finished.\n");
      }
      break;

    case 'q': /* Quit */
      exit(EXIT_SUCCESS);
      break;

    case '1':
      fprintf(stdout, "CPU Trace:\n");
      cpu_trace_dump(stdout);
      break;

    case '2':
      fprintf(stdout, "Zero Page:\n");
      mem_dump(stdout, &main_mem, 0, 0xFF);
      fprintf(stdout, "\nStack:\n");
      mem_dump(stdout, &main_mem, MEM_PAGE_STACK, MEM_PAGE_STACK + 0xFF);
      fprintf(stdout, "\nCartridge Vectors:\n");
      mem_dump(stdout, &main_mem, 0xFFF0, 0xFFFF);
      break;

    case '3':
      fprintf(stdout, "PPU Dump:\n");
      ppu_dump(stdout, &main_ppu);
      for (i = 0; i < 4; i++) {
        fprintf(stdout, "\nPPU Name Table #%d:\n", i);
        ppu_name_table_dump(stdout, &main_ppu, i);
        fprintf(stdout, "\nPPU Attribute Table #%d:\n", i);
        ppu_attribute_table_dump(stdout, &main_ppu, i);
      }
      fprintf(stdout, "\nPPU Palette RAM:\n");
      ppu_palette_ram_dump(stdout, &main_ppu);
      fprintf(stdout, "\nPPU Sprite RAM:\n");
      ppu_sprite_ram_dump(stdout, &main_ppu);
      break;

    case '4':
      for (i = 0; i < 256; i++) {
        fprintf(stdout, "--- 0:0x%02x\n", i);
        ppu_pattern_table_dump(stdout, &main_ppu, 0, i);
      }
      for (i = 0; i < 256; i++) {
        fprintf(stdout, "--- 1:0x%02x\n", i);
        ppu_pattern_table_dump(stdout, &main_ppu, 1, i);
      }
      break;

    case '5':
      apu_dump(stdout, &main_apu);
      break;

    case '6':
      fprintf(stdout, "Other RAM:\n");
      mem_dump(stdout, &main_mem, 0x200, 0x7FF);
      break;

    case '7':
      fds_dump(stdout, &main_fds);
      break;

    default:
      continue;
    }
  }
}



static void sig_handler(int sig)
{
  (void)sig;
  debugger_break = true;
}



void panic(const char *format, ...)
{
  va_list args;

  cli_pause(); /* Turn off to let text appear. */

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  debugger_break = true;
}



void debug(void)
{
  debugger_break = true;
}



static void display_help(const char *progname)
{
  fprintf(stdout, "Usage: %s <options> [rom]\n", progname);
  fprintf(stdout, "Options:\n"
    "  -h        Display this help.\n"
    "  -d        Break into debugger on start.\n"
    "  -v        Disable SDL video.\n"
    "  -a        Disable SDL audio.\n"
    "  -k        Disable terminal output.\n"
    "  -c        Disable terminal colors.\n"
    "  -j NO     Use SDL joystick NO instead of 0.\n"
    "  -t FILE   Use FM2 FILE as input for TAS.\n"
    "  -f FILE   Enable Famicom Disk System and use FILE as FDS BIOS.\n"
    "  -b        Enable BASIC mode with keyboard and data recorder."
    "\n");
}



int main(int argc, char *argv[])
{
  int c;
  char *rom_filename = NULL;
  char *tas_filename = NULL;
  char *fds_bios_filename = NULL;
  bool disable_video = false;
  bool disable_audio = false;
  bool disable_terminal = false;
  bool enable_colors = true;
  bool basic_mode = false;
  int joystick_no = 0;

  while ((c = getopt(argc, argv, "hdvakcj:t:f:b")) != -1) {
    switch (c) {
    case 'h':
      display_help(argv[0]);
      return EXIT_SUCCESS;

    case 'd':
      debugger_break = true;
      break;

    case 'v':
      disable_video = true;
      break;

    case 'a':
      disable_audio = true;
      break;

    case 'k':
      disable_terminal = true;
      break;

    case 'c':
      enable_colors = false;
      break;

    case 'j':
      joystick_no = atoi(optarg);
      break;

    case 't':
      tas_filename = optarg;
      break;

    case 'f':
      fds_bios_filename = optarg;
      break;

    case 'b':
      basic_mode = true;
      break;

    case '?':
    default:
      display_help(argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (argc <= optind) {
    display_help(argv[0]);
    return EXIT_FAILURE;
  } else {
    rom_filename = argv[optind];
  }

  cpu_trace_init();
  signal(SIGINT, sig_handler);

  mem_init(&main_mem);
  ppu_init(&main_ppu, &main_mem);
  apu_init(&main_apu, &main_mem);
  kbd_init();

  if (fds_bios_filename != NULL) {
    /* Famicom Disk System */
    fds_init(&main_fds, &main_mem, &main_ppu);
    if (fds_bios_load(fds_bios_filename, &main_mem) != 0) {
      fprintf(stderr, "Unable to load FDS BIOS: %s\n", fds_bios_filename);
      return EXIT_FAILURE;
    }
    if (fds_image_load(rom_filename, &main_fds) != 0) {
      fprintf(stderr, "Unable to load FDS image: %s\n", rom_filename);
      return EXIT_FAILURE;
    }

  } else {
    /* Regular ROM */
    if (ines_load(rom_filename, &main_mem, &main_ppu) != 0) {
      fprintf(stderr, "Unable to load ROM: %s\n", rom_filename);
      return EXIT_FAILURE;
    }
  }

  if (tas_filename != NULL) {
    if (tas_init(tas_filename) != 0) {
      fprintf(stderr, "Failed to load TAS file: %s\n", tas_filename);
      return EXIT_FAILURE;
    }
  }

  if (gui_init(joystick_no, disable_video, disable_audio, basic_mode) != 0) {
    fprintf(stderr, "Failed to initialize GUI!\n");
    return EXIT_FAILURE;
  }

  if (! disable_terminal) {
    if (cli_init(enable_colors, basic_mode) != 0) {
      fprintf(stderr, "Failed to initialize CLI!\n");
      return EXIT_FAILURE;
    }
  }

  cpu_reset(&main_cpu, &main_mem);
  while (1) {
    cpu_trace_add(&main_cpu, &main_mem);

    /* Let the PPU execute for 2 frames before the CPU starts. */
    if (main_ppu.frame_no < 2) {
      ppu_execute(&main_ppu);
    } else {
      cpu_execute(&main_cpu, &main_mem);
    }

    /* Limit PPU execution to three times per CPU cycle spent. */
    while (main_cpu.cycles > 0) {
      ppu_execute(&main_ppu);
      ppu_execute(&main_ppu);
      ppu_execute(&main_ppu);
      fds_execute(&main_fds);
      kbd_cassette_execute(main_apu.keyboard_cassette_dac,
        &main_apu.keyboard_cassette_adc);
      main_cpu.cycles--;
    }

    apu_execute(&main_apu);

    /* Trigger pending IRQ from FDS once CPU is ready. */
    if (main_fds.trigger_irq && (main_cpu.sr.i == false)) {
      cpu_irq(&main_cpu, &main_mem);
      main_fds.trigger_irq = false;
    }

    /* Check for vertical blank NMI from PPU. */
    if (main_ppu.trigger_nmi) {
      cpu_nmi(&main_cpu, &main_mem);
      main_ppu.trigger_nmi = false;
      kbd_key_clear();
      gui_update();
#ifdef EXTRA_INFO
      cli_update(&main_mem, &main_ppu, &main_apu);
#else
      cli_update();
#endif
      tas_update(main_ppu.frame_no);

      if (nmi_break) {
        nmi_break = false;
        debugger_break = true;
      }

      if (gui_save_state_requested()) {
        memcpy(&save_cpu, &main_cpu, sizeof(cpu_t));
        memcpy(&save_mem, &main_mem, sizeof(mem_t));
        memcpy(&save_ppu, &main_ppu, sizeof(ppu_t));
        memcpy(&save_apu, &main_apu, sizeof(apu_t));
        memcpy(&save_fds, &main_fds, sizeof(fds_t));
        saved_state = true;
      } else if (gui_load_state_requested() && saved_state) {
        memcpy(&main_cpu, &save_cpu, sizeof(cpu_t));
        memcpy(&main_mem, &save_mem, sizeof(mem_t));
        memcpy(&main_ppu, &save_ppu, sizeof(ppu_t));
        memcpy(&main_apu, &save_apu, sizeof(apu_t));
        memcpy(&main_fds, &save_fds, sizeof(fds_t));
      }
    }

    if (debugger_break) {
      cli_pause();
      debugger_break = debugger();
      if (! debugger_break) {
        cli_resume();
      }
    }
  }

  return EXIT_SUCCESS;
}



