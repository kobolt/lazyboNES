#ifndef _CPU_H
#define _CPU_H

#include <stdint.h>
#include <stdbool.h>
#include "mem.h"

typedef struct cpu_status_s {
  uint8_t n : 1; /* Negative */
  uint8_t v : 1; /* Overflow */
  uint8_t b : 1; /* Break */
  uint8_t d : 1; /* Decimal */
  uint8_t i : 1; /* Interrupt Disable */
  uint8_t z : 1; /* Zero */
  uint8_t c : 1; /* Carry */
} cpu_status_t;

typedef struct cpu_s {
  uint16_t pc;     /* Program Counter */
  uint8_t a;       /* Accumulator */
  uint8_t x;       /* X Register */
  uint8_t y;       /* Y Register */
  uint8_t sp;      /* Stack Pointer */
  cpu_status_t sr; /* Status Register */
  uint32_t cycles; /* Internal Cycle Counter */
} cpu_t;

typedef bool (*cpu_opcode_handler_t)(uint32_t, cpu_t *, mem_t *);

void cpu_reset(cpu_t *cpu, mem_t *mem);
void cpu_execute(cpu_t *cpu, mem_t *mem);
void cpu_trap_opcode(uint8_t opcode, cpu_opcode_handler_t handler);
void cpu_nmi(cpu_t *cpu, mem_t *mem);

void cpu_trace_init(void);
void cpu_trace_add(cpu_t *cpu, mem_t *mem);
void cpu_trace_dump(FILE *fh);

#endif /* _CPU_H */
