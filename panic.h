#ifndef _PANIC_H
#define _PANIC_H

#include <stdarg.h>

void panic(const char *format, ...);
void debug(void);

#endif /* _PANIC_H */
