#ifndef _TAS_H
#define _TAS_H

#include <stdint.h>
#include <stdbool.h>

int tas_init(const char *filename);
uint8_t tas_get_controller_state(void);
void tas_update(uint32_t frame_no);
bool tas_is_active(void);

#endif /* _TAS_H */
