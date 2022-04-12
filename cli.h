#ifndef _CLI_H
#define _CLI_H

#include <stdint.h>
#include <stdbool.h>

int cli_init(bool enable_colors);
void cli_draw_tile(uint8_t y, uint8_t x, bool table_no, uint8_t tile);
uint8_t cli_get_controller_state(void);
void cli_update(void);
void cli_pause(void);
void cli_resume(void);

#endif /* _CLI_H */
