#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "tas.h"



#define TAS_DATA_MAX 131072



static uint8_t tas_controller_state[TAS_DATA_MAX];
static unsigned int tas_data_index = 0;
static bool tas_active = false;



int tas_init(const char *filename)
{
  int n, result;
  FILE *fh;
  char buffer[32];
  uint8_t controller_state;
  char cmd, r, l, d, u, t, s, b, a;

  fh = fopen(filename, "r");
  if (fh == NULL) {
    return -1;
  }

  tas_data_index = 0;
  tas_active = true;

  n = 0;
  while (fgets(buffer, sizeof(buffer), fh) != NULL) {
    /* Simple parsing of the FM2 format. */
    result = sscanf(buffer, "|%c|%c%c%c%c%c%c%c%c|||\n",
      &cmd, &r, &l, &d, &u, &t, &s, &b, &a);

    if (result == 9) {
      controller_state = 0;
      if (r == 'R') controller_state += 0x80;
      if (l == 'L') controller_state += 0x40;
      if (d == 'D') controller_state += 0x20;
      if (u == 'U') controller_state += 0x10;
      if (t == 'T') controller_state += 0x08;
      if (s == 'S') controller_state += 0x04;
      if (b == 'B') controller_state += 0x02;
      if (a == 'A') controller_state += 0x01;

      tas_controller_state[n] = controller_state;
      n++;
      if (n >= TAS_DATA_MAX) {
        fprintf(stderr, "Overflow in TAS data.\n");
        break;
      }
    }
  }

  fclose(fh);
  return 0;
}



uint8_t tas_get_controller_state(void)
{
  if (tas_active) {
    return tas_controller_state[tas_data_index];
  } else {
    return 0;
  }
}



void tas_update(uint32_t frame_no)
{
  if (! tas_active) {
    return;
  }

  tas_data_index = frame_no;

  if (tas_data_index >= TAS_DATA_MAX) {
    tas_data_index = 0;
    tas_active = false;
  }
}



bool tas_is_active(void)
{
  return tas_active;
}



