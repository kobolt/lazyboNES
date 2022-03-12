#ifndef _INES_H
#define _INES_H

#include "mem.h"
#include "ppu.h"

int ines_load(const char *filename, mem_t *mem, ppu_t *ppu);

#endif /* _INES_H */
