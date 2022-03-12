#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "mem.h"
#include "panic.h"



typedef enum {
  AM_ACCU, /* A      - Accumulator */
  AM_IMPL, /* i      - Implied */
  AM_IMM,  /* #      - Immediate */
  AM_ABS,  /* a      - Absolute */
  AM_ABSI, /* (a)    - Indirect Absolute */
  AM_ABSX, /* a,x    - Absolute + X */
  AM_ABSY, /* a,y    - Absolute + Y */
  AM_REL,  /* r      - Relative */
  AM_ZP,   /* zp     - Zero Page */
  AM_ZPX,  /* zp,x   - Zero Page + X */
  AM_ZPY,  /* zp,y   - Zero Page + Y */
  AM_ZPYI, /* (zp),y - Zero Page Indirect Indexed */
  AM_ZPIX, /* (zp,x) - Zero Page Indexed Indirect */
  AM_NONE,
} cpu_address_mode_t;

typedef struct cpu_trace_s {
  cpu_t cpu;
  uint8_t mc[3];
} cpu_trace_t;

#define CPU_TRACE_BUFFER_SIZE 20



static cpu_address_mode_t opcode_address_mode[UINT8_MAX + 1] = {
  AM_IMPL, AM_ZPIX, AM_NONE, AM_NONE, AM_NONE, AM_ZP,   AM_ZP,   AM_NONE,
  AM_IMPL, AM_IMM,  AM_ACCU, AM_NONE, AM_NONE, AM_ABS,  AM_ABS,  AM_NONE,
  AM_REL,  AM_ZPYI, AM_NONE, AM_NONE, AM_NONE, AM_ZPX,  AM_ZPX,  AM_NONE,
  AM_IMPL, AM_ABSY, AM_NONE, AM_NONE, AM_NONE, AM_ABSX, AM_ABSX, AM_NONE,
  AM_ABS,  AM_ZPIX, AM_NONE, AM_NONE, AM_ZP,   AM_ZP,   AM_ZP,   AM_NONE,
  AM_IMPL, AM_IMM,  AM_ACCU, AM_NONE, AM_ABS,  AM_ABS,  AM_ABS,  AM_NONE,
  AM_REL,  AM_ZPYI, AM_NONE, AM_NONE, AM_NONE, AM_ZPX,  AM_ZPX,  AM_NONE,
  AM_IMPL, AM_ABSY, AM_NONE, AM_NONE, AM_NONE, AM_ABSX, AM_ABSX, AM_NONE,
  AM_IMPL, AM_ZPIX, AM_NONE, AM_NONE, AM_NONE, AM_ZP,   AM_ZP,   AM_NONE,
  AM_IMPL, AM_IMM,  AM_ACCU, AM_NONE, AM_ABS,  AM_ABS,  AM_ABS,  AM_NONE,
  AM_REL,  AM_ZPYI, AM_NONE, AM_NONE, AM_NONE, AM_ZPX,  AM_ZPX,  AM_NONE,
  AM_IMPL, AM_ABSY, AM_NONE, AM_NONE, AM_NONE, AM_ABSX, AM_ABSX, AM_NONE,
  AM_IMPL, AM_ZPIX, AM_NONE, AM_NONE, AM_NONE, AM_ZP,   AM_ZP,   AM_NONE,
  AM_IMPL, AM_IMM,  AM_ACCU, AM_NONE, AM_ABSI, AM_ABS,  AM_ABS,  AM_NONE,
  AM_REL,  AM_ZPYI, AM_NONE, AM_NONE, AM_NONE, AM_ZPX,  AM_ZPX,  AM_NONE,
  AM_IMPL, AM_ABSY, AM_NONE, AM_NONE, AM_NONE, AM_ABSX, AM_ABSX, AM_NONE,
  AM_NONE, AM_ZPIX, AM_NONE, AM_NONE, AM_ZP,   AM_ZP,   AM_ZP,   AM_NONE,
  AM_IMPL, AM_NONE, AM_IMPL, AM_NONE, AM_ABS,  AM_ABS,  AM_ABS,  AM_NONE,
  AM_REL,  AM_ZPYI, AM_NONE, AM_NONE, AM_ZPX,  AM_ZPX,  AM_ZPY,  AM_NONE,
  AM_IMPL, AM_ABSY, AM_IMPL, AM_NONE, AM_NONE, AM_ABSX, AM_NONE, AM_NONE,
  AM_IMM,  AM_ZPIX, AM_IMM,  AM_NONE, AM_ZP,   AM_ZP,   AM_ZP,   AM_NONE,
  AM_IMPL, AM_IMM,  AM_IMPL, AM_NONE, AM_ABS,  AM_ABS,  AM_ABS,  AM_NONE,
  AM_REL,  AM_ZPYI, AM_NONE, AM_NONE, AM_ZPX,  AM_ZPX,  AM_ZPY,  AM_NONE,
  AM_IMPL, AM_ABSY, AM_IMPL, AM_NONE, AM_ABSX, AM_ABSX, AM_ABSY, AM_NONE,
  AM_IMM,  AM_ZPIX, AM_NONE, AM_NONE, AM_ZP,   AM_ZP,   AM_ZP,   AM_NONE,
  AM_IMPL, AM_IMM,  AM_IMPL, AM_NONE, AM_ABS,  AM_ABS,  AM_ABS,  AM_NONE,
  AM_REL,  AM_ZPYI, AM_NONE, AM_NONE, AM_NONE, AM_ZPX,  AM_ZPX,  AM_NONE,
  AM_IMPL, AM_ABSY, AM_NONE, AM_NONE, AM_NONE, AM_ABSX, AM_ABSX, AM_NONE,
  AM_IMM,  AM_ZPIX, AM_NONE, AM_NONE, AM_ZP,   AM_ZP,   AM_ZP,   AM_NONE,
  AM_IMPL, AM_IMM,  AM_IMPL, AM_NONE, AM_ABS,  AM_ABS,  AM_ABS,  AM_NONE,
  AM_REL,  AM_ZPYI, AM_NONE, AM_NONE, AM_NONE, AM_ZPX,  AM_ZPX,  AM_NONE,
  AM_IMPL, AM_ABSY, AM_NONE, AM_NONE, AM_NONE, AM_ABSX, AM_ABSX, AM_NONE,
};

static char *opcode_mnemonic[UINT8_MAX + 1] = {
  "BRK", "ORA", "---", "---", "---", "ORA", "ASL", "---", 
  "PHP", "ORA", "ASL", "---", "---", "ORA", "ASL", "---",
  "BPL", "ORA", "---", "---", "---", "ORA", "ASL", "---",
  "CLC", "ORA", "---", "---", "---", "ORA", "ASL", "---",
  "JSR", "AND", "---", "---", "BIT", "AND", "ROL", "---",
  "PLP", "AND", "ROL", "---", "BIT", "AND", "ROL", "---",
  "BMI", "AND", "---", "---", "---", "AND", "ROL", "---",
  "SEC", "AND", "---", "---", "---", "AND", "ROL", "---",
  "RTI", "EOR", "---", "---", "---", "EOR", "LSR", "---",
  "PHA", "EOR", "LSR", "---", "JMP", "EOR", "LSR", "---",
  "BVC", "EOR", "---", "---", "---", "EOR", "LSR", "---",
  "CLI", "EOR", "---", "---", "---", "EOR", "LSR", "---",
  "RTS", "ADC", "---", "---", "---", "ADC", "ROR", "---",
  "PLA", "ADC", "ROR", "---", "JMP", "ADC", "ROR", "---",
  "BVS", "ADC", "---", "---", "---", "ADC", "ROR", "---",
  "SEI", "ADC", "---", "---", "---", "ADC", "ROR", "---",
  "---", "STA", "---", "---", "STY", "STA", "STX", "---",
  "DEY", "---", "TXA", "---", "STY", "STA", "STX", "---",
  "BCC", "STA", "---", "---", "STY", "STA", "STX", "---",
  "TYA", "STA", "TXS", "---", "---", "STA", "---", "---",
  "LDY", "LDA", "LDX", "---", "LDY", "LDA", "LDX", "---",
  "TAY", "LDA", "TAX", "---", "LDY", "LDA", "LDX", "---",
  "BCS", "LDA", "---", "---", "LDY", "LDA", "LDX", "---",
  "CLV", "LDA", "TSX", "---", "LDY", "LDA", "LDX", "---",
  "CPY", "CMP", "---", "---", "CPY", "CMP", "DEC", "---",
  "INY", "CMP", "DEX", "---", "CPY", "CMP", "DEC", "---",
  "BNE", "CMP", "---", "---", "---", "CMP", "DEC", "---",
  "CLD", "CMP", "---", "---", "---", "CMP", "DEC", "---",
  "CPX", "SBC", "---", "---", "CPX", "SBC", "INC", "---",
  "INX", "SBC", "NOP", "---", "CPX", "SBC", "INC", "---",
  "BEQ", "SBC", "---", "---", "---", "SBC", "INC", "---",
  "SED", "SBC", "---", "---", "---", "SBC", "INC", "---",
};

static cpu_opcode_handler_t cpu_trap_opcode_handler = NULL;

static cpu_trace_t cpu_trace_buffer[CPU_TRACE_BUFFER_SIZE];
static int cpu_trace_index = 0;



static void cpu_dump_disassemble(FILE *fh, uint16_t pc, uint8_t mc[3])
{
  uint16_t address;
  int8_t relative;

  switch (opcode_address_mode[mc[0]]) {
  case AM_ACCU:
  case AM_IMPL:
    fprintf(fh, "%02X          ", mc[0]);
    break;

  case AM_IMM:
  case AM_REL:
  case AM_ZP:
  case AM_ZPX:
  case AM_ZPY:
  case AM_ZPYI:
  case AM_ZPIX:
    fprintf(fh, "%02X %02X       ", mc[0], mc[1]);
    break;

  case AM_ABS:
  case AM_ABSI:
  case AM_ABSX:
  case AM_ABSY:
    fprintf(fh, "%02X %02X %02X    ", mc[0], mc[1], mc[2]);
    break;

  case AM_NONE:
  default:
    fprintf(fh, "-       ");
    break;
  }

  fprintf(fh, "%s ", opcode_mnemonic[mc[0]]);

  switch (opcode_address_mode[mc[0]]) {
  case AM_ACCU:
    fprintf(fh, "A       ");
    break;

  case AM_IMPL:
    fprintf(fh, "        ");
    break;

  case AM_IMM:
    fprintf(fh, "#$%02X    ", mc[1]);
    break;

  case AM_ABS:
    fprintf(fh, "$%02X%02X   ", mc[2], mc[1]);
    break;

  case AM_ABSI:
    fprintf(fh, "($%02X%02X) ", mc[2], mc[1]);
    break;

  case AM_ABSX:
    fprintf(fh, "$%02X%02X,X ", mc[2], mc[1]);
    break;

  case AM_ABSY:
    fprintf(fh, "$%02X%02X,Y ", mc[2], mc[1]);
    break;

  case AM_REL:
    address = pc + 2;
    relative = mc[1];
    address += relative;
    fprintf(fh, "$%04X   ", address);
    break;

  case AM_ZP:
    fprintf(fh, "$%02X     ", mc[1]);
    break;

  case AM_ZPX:
    fprintf(fh, "$%02X,X   ", mc[1]);
    break;

  case AM_ZPY:
    fprintf(fh, "$%02X,Y   ", mc[1]);
    break;

  case AM_ZPYI:
    fprintf(fh, "($%02X),Y ", mc[1]);
    break;

  case AM_ZPIX:
    fprintf(fh, "($%02X,X) ", mc[1]);
    break;

  case AM_NONE:
  default:
    fprintf(fh, "-       ");
    break;
  }
}



void cpu_trap_opcode(uint8_t opcode, cpu_opcode_handler_t handler)
{
  opcode_mnemonic[opcode] = "TRP";
  opcode_address_mode[opcode] = AM_IMPL;
  cpu_trap_opcode_handler = handler;
}



static void cpu_register_dump(FILE *fh, cpu_t *cpu, uint8_t mc[3])
{
  fprintf(fh, ".C:%04x  ", cpu->pc);
  cpu_dump_disassemble(fh, cpu->pc, mc);
  fprintf(fh, "   - ");
  fprintf(fh, "A:%02X ", cpu->a);
  fprintf(fh, "X:%02X ", cpu->x);
  fprintf(fh, "Y:%02X ", cpu->y);
  fprintf(fh, "SP:%02x ", cpu->sp);
  fprintf(fh, "%c", (cpu->sr.n) ? 'N' : '.');
  fprintf(fh, "%c", (cpu->sr.v) ? 'V' : '.');
  fprintf(fh, "-");
  fprintf(fh, "%c", (cpu->sr.b) ? 'B' : '.');
  fprintf(fh, "%c", (cpu->sr.d) ? 'D' : '.');
  fprintf(fh, "%c", (cpu->sr.i) ? 'I' : '.');
  fprintf(fh, "%c", (cpu->sr.z) ? 'Z' : '.');
  fprintf(fh, "%c", (cpu->sr.c) ? 'C' : '.');
  fprintf(fh, "\n");
}



void cpu_trace_init(void)
{
  memset(cpu_trace_buffer, 0, CPU_TRACE_BUFFER_SIZE * sizeof(cpu_trace_t));
}



void cpu_trace_dump(FILE *fh)
{
  int i;

  for (i = 0; i < CPU_TRACE_BUFFER_SIZE; i++) {
    cpu_trace_index++;
    if (cpu_trace_index >= CPU_TRACE_BUFFER_SIZE) {
      cpu_trace_index = 0;
    }
    cpu_register_dump(fh, &cpu_trace_buffer[cpu_trace_index].cpu,
                           cpu_trace_buffer[cpu_trace_index].mc);
  }
}



void cpu_trace_add(cpu_t *cpu, mem_t *mem)
{
  uint8_t mc[3];

  cpu_trace_index++;
  if (cpu_trace_index >= CPU_TRACE_BUFFER_SIZE) {
    cpu_trace_index = 0;
  }

  memcpy(&cpu_trace_buffer[cpu_trace_index].cpu, cpu, sizeof(cpu_t));
  mc[0] = mem_read(mem, cpu->pc);
  mc[1] = mem_read(mem, cpu->pc + 1);
  mc[2] = mem_read(mem, cpu->pc + 2);
  memcpy(&cpu_trace_buffer[cpu_trace_index].mc, mc, sizeof(uint8_t) * 3);
}



static uint8_t cpu_status_get(cpu_t *cpu)
{
  /* NOTE: Break flag is set, as needed by PHP and BRK. */
  return ((cpu->sr.n << 7) +
          (cpu->sr.v << 6) +
          (1         << 5) +
          (1         << 4) +
          (cpu->sr.d << 3) +
          (cpu->sr.i << 2) +
          (cpu->sr.z << 1) +
           cpu->sr.c);
}



static void cpu_status_set(cpu_t *cpu, uint8_t flags)
{
  /* NOTE: Break flag is discarded, as needed by PLP and RTI. */
  cpu->sr.n = (flags >> 7) & 0x1;
  cpu->sr.v = (flags >> 6) & 0x1;
  cpu->sr.d = (flags >> 3) & 0x1;
  cpu->sr.i = (flags >> 2) & 0x1;
  cpu->sr.z = (flags >> 1) & 0x1;
  cpu->sr.c =  flags       & 0x1;
}



static inline uint8_t dec_to_bin(uint8_t value)
{
  return (value % 0x10) + ((value / 0x10) * 10);
}

static inline uint8_t bin_to_dec(uint8_t value)
{
  return (value % 10) + ((value / 10) * 0x10);
}



static inline void cpu_flag_zero_other(cpu_t *cpu, uint8_t value)
{
  if (value == 0) {
    cpu->sr.z = 1;
  } else {
    cpu->sr.z = 0;
  }
}

static inline void cpu_flag_negative_other(cpu_t *cpu, uint8_t value)
{
  if ((value >> 7) == 1) {
    cpu->sr.n = 1;
  } else {
    cpu->sr.n = 0;
  }
}

static inline void cpu_flag_zero_compare(cpu_t *cpu, uint8_t a, uint8_t b)
{
  if (a == b) {
    cpu->sr.z = 1;
  } else {
    cpu->sr.z = 0;
  }
}

static inline void cpu_flag_negative_compare(cpu_t *cpu, uint8_t a, uint8_t b)
{
  if (((uint8_t)(a - b) >> 7) == 1) {
    cpu->sr.n = 1;
  } else {
    cpu->sr.n = 0;
  }
}

static inline void cpu_flag_carry_compare(cpu_t *cpu, uint8_t a, uint8_t b)
{
  if (a >= b) {
    cpu->sr.c = 1;
  } else {
    cpu->sr.c = 0;
  }
}

static inline bool cpu_flag_carry_add(cpu_t *cpu, uint8_t value)
{
  if (cpu->sr.d == 1) {
    if (cpu->a + value + cpu->sr.c > 99) {
      return 1;
    } else {
      return 0;
    }
  } else {
    if (cpu->a + value + cpu->sr.c > 0xFF) {
      return 1;
    } else {
      return 0;
    }
  }
}

static inline bool cpu_flag_carry_sub(cpu_t *cpu, uint8_t value)
{
  bool borrow;
  if (cpu->sr.c == 0) {
    borrow = 1;
  } else {
    borrow = 0;
  }
  if (cpu->a - value - borrow < 0) {
    return 0;
  } else {
    return 1;
  }
}

static inline void cpu_flag_overflow_add(cpu_t *cpu, uint8_t a, uint8_t b)
{
  if (a >> 7 == b >> 7) {
    if (cpu->a >> 7 != a >> 7) {
      cpu->sr.v = 1;
    } else {
      cpu->sr.v = 0;
    }
  } else {
    cpu->sr.v = 0;
  }
}

static inline void cpu_flag_overflow_sub(cpu_t *cpu, uint8_t a, uint8_t b)
{
  if (a >> 7 != b >> 7) {
    if (cpu->a >> 7 != a >> 7) {
      cpu->sr.v = 1;
    } else {
      cpu->sr.v = 0;
    }
  } else {
    cpu->sr.v = 0;
  }
}

static inline void cpu_flag_overflow_bit(cpu_t *cpu, uint8_t value)
{
  if (((value >> 6) & 0x1) == 1) {
    cpu->sr.v = 1;
  } else {
    cpu->sr.v = 0;
  }
}



#define OP_PROLOGUE_ABS \
  uint16_t absolute; \
  absolute  = mem_read(mem, cpu->pc++); \
  absolute += mem_read(mem, cpu->pc++) * 256;

#define OP_PROLOGUE_ABSX \
  uint16_t absolute; \
  absolute  = mem_read(mem, cpu->pc++); \
  absolute += mem_read(mem, cpu->pc++) * 256; \
  absolute += cpu->x;

#define OP_PROLOGUE_ABSY \
  uint16_t absolute; \
  absolute  = mem_read(mem, cpu->pc++); \
  absolute += mem_read(mem, cpu->pc++) * 256; \
  absolute += cpu->y; \

#define OP_PROLOGUE_ZP \
  uint8_t zeropage; \
  zeropage = mem_read(mem, cpu->pc++); \

#define OP_PROLOGUE_ZPX \
  uint8_t zeropage; \
  zeropage  = mem_read(mem, cpu->pc++); \
  zeropage += cpu->x;

#define OP_PROLOGUE_ZPY \
  uint8_t zeropage; \
  zeropage  = mem_read(mem, cpu->pc++); \
  zeropage += cpu->y;

#define OP_PROLOGUE_ZPYI \
  uint8_t zeropage; \
  uint16_t absolute; \
  zeropage  = mem_read(mem, cpu->pc++); \
  absolute  = mem_read(mem, zeropage); \
  zeropage += 1; \
  absolute += mem_read(mem, zeropage) * 256; \
  absolute += cpu->y;

#define OP_PROLOGUE_ZPIX \
  uint8_t zeropage; \
  uint16_t absolute; \
  zeropage  = mem_read(mem, cpu->pc++); \
  zeropage += cpu->x; \
  absolute  = mem_read(mem, zeropage); \
  zeropage += 1; \
  absolute += mem_read(mem, zeropage) * 256;



static inline void cpu_logic_adc(cpu_t *cpu, uint8_t value)
{
  uint8_t initial;
  bool bit;
  if (cpu->sr.d == 1) {
    value = dec_to_bin(value);
    cpu->a = dec_to_bin(cpu->a);
  }
  initial = cpu->a;
  bit = cpu_flag_carry_add(cpu, value);
  cpu->a += value;
  cpu->a += cpu->sr.c;
  if (cpu->sr.d == 1) {
    if (cpu->a > 99) {
      cpu->a %= 100;
    }
    cpu->a = bin_to_dec(cpu->a);
  }
  cpu->sr.c = bit;
  cpu_flag_overflow_add(cpu, initial, value);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static inline void cpu_logic_sbc(cpu_t *cpu, uint8_t value)
{
  uint8_t initial;
  bool bit;
  if (cpu->sr.d == 1) {
    value = dec_to_bin(value);
    cpu->a = dec_to_bin(cpu->a);
  }
  initial = cpu->a;
  bit = cpu_flag_carry_sub(cpu, value);
  cpu->a -= value;
  if (cpu->sr.c == 0) {
    cpu->a -= 1;
  }
  if (cpu->sr.d == 1) {
    if (cpu->a > 99) {
      cpu->a -= (255 - 99);
    }
    cpu->a = bin_to_dec(cpu->a);
  }
  cpu->sr.c = bit;
  cpu_flag_overflow_sub(cpu, initial, value);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}



static void op_adc_imm(cpu_t *cpu, mem_t *mem)
{
  uint8_t value = mem_read(mem, cpu->pc++);
  cpu_logic_adc(cpu, value);
}

static void op_adc_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint8_t value = mem_read(mem, absolute);
  cpu_logic_adc(cpu, value);
}

static void op_adc_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  uint8_t value = mem_read(mem, absolute);
  cpu_logic_adc(cpu, value);
}

static void op_adc_absy(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSY
  uint8_t value = mem_read(mem, absolute);
  cpu_logic_adc(cpu, value);
}

static void op_adc_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  uint8_t value = mem_read(mem, zeropage);
  cpu_logic_adc(cpu, value);
}

static void op_adc_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  uint8_t value = mem_read(mem, zeropage);
  cpu_logic_adc(cpu, value);
}

static void op_adc_zpyi(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPYI
  uint8_t value = mem_read(mem, absolute);
  cpu_logic_adc(cpu, value);
}

static void op_adc_zpix(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPIX
  uint8_t value = mem_read(mem, absolute);
  cpu_logic_adc(cpu, value);
}

static void op_and_imm(cpu_t *cpu, mem_t *mem)
{
  cpu->a &= mem_read(mem, cpu->pc++);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_and_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  cpu->a &= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_and_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  cpu->a &= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_and_absy(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSY
  cpu->a &= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_and_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  cpu->a &= mem_read(mem, zeropage);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_and_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  cpu->a &= mem_read(mem, zeropage);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_and_zpyi(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPYI
  cpu->a &= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_and_zpix(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPIX
  cpu->a &= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_asl_accu(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  uint8_t value = cpu->a;
  bool bit = value & 0b10000000;
  value = value << 1;
  cpu->a = value;
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_asl_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint8_t value = mem_read(mem, absolute);
  bool bit = value & 0b10000000;
  value = value << 1;
  mem_write(mem, absolute, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_asl_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  uint8_t value = mem_read(mem, absolute);
  bool bit = value & 0b10000000;
  value = value << 1;
  mem_write(mem, absolute, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_asl_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  uint8_t value = mem_read(mem, zeropage);
  bool bit = value & 0b10000000;
  value = value << 1;
  mem_write(mem, zeropage, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_asl_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  uint8_t value = mem_read(mem, zeropage);
  bool bit = value & 0b10000000;
  value = value << 1;
  mem_write(mem, zeropage, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_bcc(cpu_t *cpu, mem_t *mem)
{
  int8_t relative = mem_read(mem, cpu->pc++);
  if (cpu->sr.c == 0) {
    cpu->pc += relative;
  }
}

static void op_bcs(cpu_t *cpu, mem_t *mem)
{
  int8_t relative = mem_read(mem, cpu->pc++);
  if (cpu->sr.c == 1) {
    cpu->pc += relative;
  }
}

static void op_beq(cpu_t *cpu, mem_t *mem)
{
  int8_t relative = mem_read(mem, cpu->pc++);
  if (cpu->sr.z == 1) {
    cpu->pc += relative;
  }
}

static void op_bit_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint8_t value = mem_read(mem, absolute);
  cpu_flag_overflow_bit(cpu, value);
  cpu_flag_negative_other(cpu, value);
  value &= cpu->a;
  cpu_flag_zero_other(cpu, value);
}

static void op_bit_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  uint8_t value = mem_read(mem, zeropage);
  cpu_flag_overflow_bit(cpu, value);
  cpu_flag_negative_other(cpu, value);
  value &= cpu->a;
  cpu_flag_zero_other(cpu, value);
}

static void op_bmi(cpu_t *cpu, mem_t *mem)
{
  int8_t relative = mem_read(mem, cpu->pc++);
  if (cpu->sr.n == 1) {
    cpu->pc += relative;
  }
}

static void op_bne(cpu_t *cpu, mem_t *mem)
{
  int8_t relative = mem_read(mem, cpu->pc++);
  if (cpu->sr.z == 0) {
    cpu->pc += relative;
  }
}

static void op_bpl(cpu_t *cpu, mem_t *mem)
{
  int8_t relative = mem_read(mem, cpu->pc++);
  if (cpu->sr.n == 0) {
    cpu->pc += relative;
  }
}

static void op_brk(cpu_t *cpu, mem_t *mem)
{
  mem_write(mem, MEM_PAGE_STACK + cpu->sp--, (cpu->pc + 1) / 256);
  mem_write(mem, MEM_PAGE_STACK + cpu->sp--, (cpu->pc + 1) % 256);
  mem_write(mem, MEM_PAGE_STACK + cpu->sp--, cpu_status_get(cpu));
  cpu->sr.i = 1;
  cpu->sr.b = 1; /* Set for visualization only, not really used. */
  cpu->pc  = mem_read(mem, MEM_VECTOR_IRQ_LOW);
  cpu->pc += mem_read(mem, MEM_VECTOR_IRQ_HIGH) * 256;
}

static void op_bvc(cpu_t *cpu, mem_t *mem)
{
  int8_t relative = mem_read(mem, cpu->pc++);
  if (cpu->sr.v == 0) {
    cpu->pc += relative;
  }
}

static void op_bvs(cpu_t *cpu, mem_t *mem)
{
  int8_t relative = mem_read(mem, cpu->pc++);
  if (cpu->sr.v == 1) {
    cpu->pc += relative;
  }
}

static void op_clc(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->sr.c = 0;
}

static void op_cld(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->sr.d = 0;
}

static void op_cli(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->sr.i = 0;
}

static void op_clv(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->sr.v = 0;
}

static void op_cmp_imm(cpu_t *cpu, mem_t *mem)
{
  uint8_t value = mem_read(mem, cpu->pc++);
  cpu_flag_negative_compare(cpu, cpu->a, value);
  cpu_flag_zero_compare(cpu, cpu->a, value);
  cpu_flag_carry_compare(cpu, cpu->a, value);
}

static void op_cmp_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint8_t value = mem_read(mem, absolute);
  cpu_flag_negative_compare(cpu, cpu->a, value);
  cpu_flag_zero_compare(cpu, cpu->a, value);
  cpu_flag_carry_compare(cpu, cpu->a, value);
}

static void op_cmp_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  uint8_t value = mem_read(mem, absolute);
  cpu_flag_negative_compare(cpu, cpu->a, value);
  cpu_flag_zero_compare(cpu, cpu->a, value);
  cpu_flag_carry_compare(cpu, cpu->a, value);
}

static void op_cmp_absy(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSY
  uint8_t value = mem_read(mem, absolute);
  cpu_flag_negative_compare(cpu, cpu->a, value);
  cpu_flag_zero_compare(cpu, cpu->a, value);
  cpu_flag_carry_compare(cpu, cpu->a, value);
}

static void op_cmp_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  uint8_t value = mem_read(mem, zeropage);
  cpu_flag_negative_compare(cpu, cpu->a, value);
  cpu_flag_zero_compare(cpu, cpu->a, value);
  cpu_flag_carry_compare(cpu, cpu->a, value);
}

static void op_cmp_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  uint8_t value = mem_read(mem, zeropage);
  cpu_flag_negative_compare(cpu, cpu->a, value);
  cpu_flag_zero_compare(cpu, cpu->a, value);
  cpu_flag_carry_compare(cpu, cpu->a, value);
}

static void op_cmp_zpyi(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPYI
  uint8_t value = mem_read(mem, absolute);
  cpu_flag_negative_compare(cpu, cpu->a, value);
  cpu_flag_zero_compare(cpu, cpu->a, value);
  cpu_flag_carry_compare(cpu, cpu->a, value);
}

static void op_cmp_zpix(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPIX
  uint8_t value = mem_read(mem, absolute);
  cpu_flag_negative_compare(cpu, cpu->a, value);
  cpu_flag_zero_compare(cpu, cpu->a, value);
  cpu_flag_carry_compare(cpu, cpu->a, value);
}

static void op_cpx_imm(cpu_t *cpu, mem_t *mem)
{
  uint8_t value = mem_read(mem, cpu->pc++);
  cpu_flag_negative_compare(cpu, cpu->x, value);
  cpu_flag_zero_compare(cpu, cpu->x, value);
  cpu_flag_carry_compare(cpu, cpu->x, value);
}

static void op_cpx_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint8_t value = mem_read(mem, absolute);
  cpu_flag_negative_compare(cpu, cpu->x, value);
  cpu_flag_zero_compare(cpu, cpu->x, value);
  cpu_flag_carry_compare(cpu, cpu->x, value);
}

static void op_cpx_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  uint8_t value = mem_read(mem, zeropage);
  cpu_flag_negative_compare(cpu, cpu->x, value);
  cpu_flag_zero_compare(cpu, cpu->x, value);
  cpu_flag_carry_compare(cpu, cpu->x, value);
}

static void op_cpy_imm(cpu_t *cpu, mem_t *mem)
{
  uint8_t value = mem_read(mem, cpu->pc++);
  cpu_flag_negative_compare(cpu, cpu->y, value);
  cpu_flag_zero_compare(cpu, cpu->y, value);
  cpu_flag_carry_compare(cpu, cpu->y, value);
}

static void op_cpy_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint8_t value = mem_read(mem, absolute);
  cpu_flag_negative_compare(cpu, cpu->y, value);
  cpu_flag_zero_compare(cpu, cpu->y, value);
  cpu_flag_carry_compare(cpu, cpu->y, value);
}

static void op_cpy_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  uint8_t value = mem_read(mem, zeropage);
  cpu_flag_negative_compare(cpu, cpu->y, value);
  cpu_flag_zero_compare(cpu, cpu->y, value);
  cpu_flag_carry_compare(cpu, cpu->y, value);
}

static void op_dec_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint8_t value = mem_read(mem, absolute);
  value -= 1;
  mem_write(mem, absolute, value);
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_dec_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  uint8_t value = mem_read(mem, absolute);
  value -= 1;
  mem_write(mem, absolute, value);
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_dec_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  uint8_t value = mem_read(mem, zeropage);
  value -= 1;
  mem_write(mem, zeropage, value);
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_dec_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  uint8_t value = mem_read(mem, zeropage);
  value -= 1;
  mem_write(mem, zeropage, value);
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_dex(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->x--;
  cpu_flag_negative_other(cpu, cpu->x);
  cpu_flag_zero_other(cpu, cpu->x);
}

static void op_dey(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->y--;
  cpu_flag_negative_other(cpu, cpu->y);
  cpu_flag_zero_other(cpu, cpu->y);
}

static void op_eor_imm(cpu_t *cpu, mem_t *mem)
{
  cpu->a ^= mem_read(mem, cpu->pc++);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_eor_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  cpu->a ^= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_eor_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  cpu->a ^= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_eor_absy(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSY
  cpu->a ^= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_eor_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  cpu->a ^= mem_read(mem, zeropage);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_eor_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  cpu->a ^= mem_read(mem, zeropage);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_eor_zpyi(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPYI
  cpu->a ^= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_eor_zpix(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPIX
  cpu->a ^= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_inc_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint8_t value = mem_read(mem, absolute);
  value += 1;
  mem_write(mem, absolute, value);
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_inc_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  uint8_t value = mem_read(mem, absolute);
  value += 1;
  mem_write(mem, absolute, value);
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_inc_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  uint8_t value = mem_read(mem, zeropage);
  value += 1;
  mem_write(mem, zeropage, value);
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_inc_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  uint8_t value = mem_read(mem, zeropage);
  value += 1;
  mem_write(mem, zeropage, value);
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_inx(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->x++;
  cpu_flag_negative_other(cpu, cpu->x);
  cpu_flag_zero_other(cpu, cpu->x);
}

static void op_iny(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->y++;
  cpu_flag_negative_other(cpu, cpu->y);
  cpu_flag_zero_other(cpu, cpu->y);
}

static void op_jmp_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  cpu->pc = absolute;
}

static void op_jmp_absi(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint16_t address = mem_read(mem, absolute);
  absolute += 1;
  address += mem_read(mem, absolute) * 256;
  cpu->pc = address;
}

static void op_jsr(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  mem_write(mem, MEM_PAGE_STACK + cpu->sp--, (cpu->pc - 1) / 256);
  mem_write(mem, MEM_PAGE_STACK + cpu->sp--, (cpu->pc - 1) % 256);
  cpu->pc = absolute;
}

static void op_lda_imm(cpu_t *cpu, mem_t *mem)
{
  cpu->a = mem_read(mem, cpu->pc++);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_lda_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  cpu->a = mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_lda_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  cpu->a = mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_lda_absy(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSY
  cpu->a = mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_lda_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  cpu->a = mem_read(mem, zeropage);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_lda_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  cpu->a = mem_read(mem, zeropage);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_lda_zpyi(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPYI
  cpu->a = mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_lda_zpix(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPIX
  cpu->a = mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_ldx_imm(cpu_t *cpu, mem_t *mem)
{
  cpu->x = mem_read(mem, cpu->pc++);
  cpu_flag_negative_other(cpu, cpu->x);
  cpu_flag_zero_other(cpu, cpu->x);
}

static void op_ldx_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  cpu->x = mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->x);
  cpu_flag_zero_other(cpu, cpu->x);
}

static void op_ldx_absy(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSY
  cpu->x = mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->x);
  cpu_flag_zero_other(cpu, cpu->x);
}

static void op_ldx_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  cpu->x = mem_read(mem, zeropage);
  cpu_flag_negative_other(cpu, cpu->x);
  cpu_flag_zero_other(cpu, cpu->x);
}

static void op_ldx_zpy(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPY
  cpu->x = mem_read(mem, zeropage);
  cpu_flag_negative_other(cpu, cpu->x);
  cpu_flag_zero_other(cpu, cpu->x);
}

static void op_ldy_imm(cpu_t *cpu, mem_t *mem)
{
  cpu->y = mem_read(mem, cpu->pc++);
  cpu_flag_negative_other(cpu, cpu->y);
  cpu_flag_zero_other(cpu, cpu->y);
}

static void op_ldy_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  cpu->y = mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->y);
  cpu_flag_zero_other(cpu, cpu->y);
}

static void op_ldy_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  cpu->y = mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->y);
  cpu_flag_zero_other(cpu, cpu->y);
}

static void op_ldy_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  cpu->y = mem_read(mem, zeropage);
  cpu_flag_negative_other(cpu, cpu->y);
  cpu_flag_zero_other(cpu, cpu->y);
}

static void op_ldy_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  cpu->y = mem_read(mem, zeropage);
  cpu_flag_negative_other(cpu, cpu->y);
  cpu_flag_zero_other(cpu, cpu->y);
}

static void op_lsr_accu(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  uint8_t value = cpu->a;
  bool bit = value & 0b00000001;
  value = value >> 1;
  cpu->a = value;
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_lsr_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint8_t value = mem_read(mem, absolute);
  bool bit = value & 0b00000001;
  value = value >> 1;
  mem_write(mem, absolute, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_lsr_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  uint8_t value = mem_read(mem, absolute);
  bool bit = value & 0b00000001;
  value = value >> 1;
  mem_write(mem, absolute, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_lsr_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  uint8_t value = mem_read(mem, zeropage);
  bool bit = value & 0b00000001;
  value = value >> 1;
  mem_write(mem, zeropage, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_lsr_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  uint8_t value = mem_read(mem, zeropage);
  bool bit = value & 0b00000001;
  value = value >> 1;
  mem_write(mem, zeropage, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_nop(cpu_t *cpu, mem_t *mem)
{
  (void)cpu;
  (void)mem;
}

static void op_ora_imm(cpu_t *cpu, mem_t *mem)
{
  cpu->a |= mem_read(mem, cpu->pc++);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_ora_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  cpu->a |= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_ora_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  cpu->a |= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_ora_absy(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSY
  cpu->a |= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_ora_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  cpu->a |= mem_read(mem, zeropage);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_ora_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  cpu->a |= mem_read(mem, zeropage);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_ora_zpyi(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPYI
  cpu->a |= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_ora_zpix(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPIX
  cpu->a |= mem_read(mem, absolute);
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_pha(cpu_t *cpu, mem_t *mem)
{
  mem_write(mem, MEM_PAGE_STACK + cpu->sp--, cpu->a);
}

static void op_php(cpu_t *cpu, mem_t *mem)
{
  mem_write(mem, MEM_PAGE_STACK + cpu->sp--, cpu_status_get(cpu));
}

static void op_pla(cpu_t *cpu, mem_t *mem)
{
  cpu->a = mem_read(mem, MEM_PAGE_STACK + (++cpu->sp));
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_plp(cpu_t *cpu, mem_t *mem)
{
  cpu_status_set(cpu, mem_read(mem, MEM_PAGE_STACK + (++cpu->sp)));
}

static void op_rol_accu(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  uint8_t value = cpu->a;
  bool bit = value & 0b10000000;
  value = value << 1;
  if (cpu->sr.c == 1) {
    value |= 0b00000001;
  }
  cpu->a = value;
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_rol_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint8_t value = mem_read(mem, absolute);
  bool bit = value & 0b10000000;
  value = value << 1;
  if (cpu->sr.c == 1) {
    value |= 0b00000001;
  }
  mem_write(mem, absolute, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_rol_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  uint8_t value = mem_read(mem, absolute);
  bool bit = value & 0b10000000;
  value = value << 1;
  if (cpu->sr.c == 1) {
    value |= 0b00000001;
  }
  mem_write(mem, absolute, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_rol_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  uint8_t value = mem_read(mem, zeropage);
  bool bit = value & 0b10000000;
  value = value << 1;
  if (cpu->sr.c == 1) {
    value |= 0b00000001;
  }
  mem_write(mem, zeropage, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_rol_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  uint8_t value = mem_read(mem, zeropage);
  bool bit = value & 0b10000000;
  value = value << 1;
  if (cpu->sr.c == 1) {
    value |= 0b00000001;
  }
  mem_write(mem, zeropage, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_ror_accu(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  uint8_t value = cpu->a;
  bool bit = value & 0b00000001;
  value = value >> 1;
  if (cpu->sr.c == 1) {
    value |= 0b10000000;
  }
  cpu->a = value;
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_ror_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint8_t value = mem_read(mem, absolute);
  bool bit = value & 0b00000001;
  value = value >> 1;
  if (cpu->sr.c == 1) {
    value |= 0b10000000;
  }
  mem_write(mem, absolute, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_ror_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  uint8_t value = mem_read(mem, absolute);
  bool bit = value & 0b00000001;
  value = value >> 1;
  if (cpu->sr.c == 1) {
    value |= 0b10000000;
  }
  mem_write(mem, absolute, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_ror_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  uint8_t value = mem_read(mem, zeropage);
  bool bit = value & 0b00000001;
  value = value >> 1;
  if (cpu->sr.c == 1) {
    value |= 0b10000000;
  }
  mem_write(mem, zeropage, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_ror_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  uint8_t value = mem_read(mem, zeropage);
  bool bit = value & 0b00000001;
  value = value >> 1;
  if (cpu->sr.c == 1) {
    value |= 0b10000000;
  }
  mem_write(mem, zeropage, value);
  cpu->sr.c = bit;
  cpu_flag_negative_other(cpu, value);
  cpu_flag_zero_other(cpu, value);
}

static void op_rti(cpu_t *cpu, mem_t *mem)
{
  cpu->sr.b = 0; /* Cleared for visualization only, not really used. */
  cpu_status_set(cpu, mem_read(mem, MEM_PAGE_STACK + (++cpu->sp)));
  cpu->pc  = mem_read(mem, MEM_PAGE_STACK + (++cpu->sp));
  cpu->pc += mem_read(mem, MEM_PAGE_STACK + (++cpu->sp)) * 256;
}

static void op_rts(cpu_t *cpu, mem_t *mem)
{
  cpu->pc  = mem_read(mem, MEM_PAGE_STACK + (++cpu->sp));
  cpu->pc += mem_read(mem, MEM_PAGE_STACK + (++cpu->sp)) * 256;
  cpu->pc += 1;
}

static void op_sbc_imm(cpu_t *cpu, mem_t *mem)
{
  uint8_t value = mem_read(mem, cpu->pc++);
  cpu_logic_sbc(cpu, value);
}

static void op_sbc_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  uint8_t value = mem_read(mem, absolute);
  cpu_logic_sbc(cpu, value);
}

static void op_sbc_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  uint8_t value = mem_read(mem, absolute);
  cpu_logic_sbc(cpu, value);
}

static void op_sbc_absy(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSY
  uint8_t value = mem_read(mem, absolute);
  cpu_logic_sbc(cpu, value);
}

static void op_sbc_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  uint8_t value = mem_read(mem, zeropage);
  cpu_logic_sbc(cpu, value);
}

static void op_sbc_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  uint8_t value = mem_read(mem, zeropage);
  cpu_logic_sbc(cpu, value);
}

static void op_sbc_zpyi(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPYI
  uint8_t value = mem_read(mem, absolute);
  cpu_logic_sbc(cpu, value);
}

static void op_sbc_zpix(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPIX
  uint8_t value = mem_read(mem, absolute);
  cpu_logic_sbc(cpu, value);
}

static void op_sec(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->sr.c = 1;
}

static void op_sed(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->sr.d = 1;
}

static void op_sei(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->sr.i = 1;
}

static void op_sta_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  mem_write(mem, absolute, cpu->a);
}

static void op_sta_absx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSX
  mem_write(mem, absolute, cpu->a);
}

static void op_sta_absy(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABSY
  mem_write(mem, absolute, cpu->a);
}

static void op_sta_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  mem_write(mem, zeropage, cpu->a);
}

static void op_sta_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  mem_write(mem, zeropage, cpu->a);
}

static void op_sta_zpyi(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPYI
  mem_write(mem, absolute, cpu->a);
}

static void op_sta_zpix(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPIX
  mem_write(mem, absolute, cpu->a);
}

static void op_stx_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  mem_write(mem, absolute, cpu->x);
}

static void op_stx_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  mem_write(mem, zeropage, cpu->x);
}

static void op_stx_zpy(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPY
  mem_write(mem, zeropage, cpu->x);
}

static void op_sty_abs(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ABS
  mem_write(mem, absolute, cpu->y);
}

static void op_sty_zp(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZP
  mem_write(mem, zeropage, cpu->y);
}

static void op_sty_zpx(cpu_t *cpu, mem_t *mem)
{
  OP_PROLOGUE_ZPX
  mem_write(mem, zeropage, cpu->y);
}

static void op_tax(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->x = cpu->a;
  cpu_flag_negative_other(cpu, cpu->x);
  cpu_flag_zero_other(cpu, cpu->x);
}

static void op_tay(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->y = cpu->a;
  cpu_flag_negative_other(cpu, cpu->y);
  cpu_flag_zero_other(cpu, cpu->y);
}

static void op_tsx(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->x = cpu->sp;
  cpu_flag_negative_other(cpu, cpu->x);
  cpu_flag_zero_other(cpu, cpu->x);
}

static void op_txa(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->a = cpu->x;
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}

static void op_txs(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->sp = cpu->x;
}

static void op_tya(cpu_t *cpu, mem_t *mem)
{
  (void)mem;
  cpu->a = cpu->y;
  cpu_flag_negative_other(cpu, cpu->a);
  cpu_flag_zero_other(cpu, cpu->a);
}



static void op_none(cpu_t *cpu, mem_t *mem)
{
  uint8_t opcode;
  opcode = mem_read(mem, cpu->pc - 1);

  if (cpu_trap_opcode_handler != NULL) {
    if (true == (cpu_trap_opcode_handler)(opcode, cpu, mem)) {
      /* Request was handled...  */
      return;
    }
  }
  panic("Panic! Unhandled opcode: %02x\n", opcode);
}

typedef void (*cpu_operation_func_t)(cpu_t *, mem_t *);

static cpu_operation_func_t opcode_function[UINT8_MAX + 1] = {
  op_brk,      op_ora_zpix, op_none,     op_none, /* 0x00 -> 0x03 */
  op_none,     op_ora_zp,   op_asl_zp,   op_none, /* 0x04 -> 0x07 */
  op_php,      op_ora_imm,  op_asl_accu, op_none, /* 0x08 -> 0x0B */
  op_none,     op_ora_abs,  op_asl_abs,  op_none, /* 0x0C -> 0x0F */
  op_bpl,      op_ora_zpyi, op_none,     op_none, /* 0x10 -> 0x13 */
  op_none,     op_ora_zpx,  op_asl_zpx,  op_none, /* 0x14 -> 0x17 */
  op_clc,      op_ora_absy, op_none,     op_none, /* 0x18 -> 0x1B */
  op_none,     op_ora_absx, op_asl_absx, op_none, /* 0x1C -> 0x1F */
  op_jsr,      op_and_zpix, op_none,     op_none, /* 0x20 -> 0x23 */
  op_bit_zp,   op_and_zp,   op_rol_zp,   op_none, /* 0x24 -> 0x27 */
  op_plp,      op_and_imm,  op_rol_accu, op_none, /* 0x28 -> 0x2B */
  op_bit_abs,  op_and_abs,  op_rol_abs,  op_none, /* 0x2C -> 0x2F */
  op_bmi,      op_and_zpyi, op_none,     op_none, /* 0x30 -> 0x33 */
  op_none,     op_and_zpx,  op_rol_zpx,  op_none, /* 0x34 -> 0x37 */
  op_sec,      op_and_absy, op_none,     op_none, /* 0x38 -> 0x3B */
  op_none,     op_and_absx, op_rol_absx, op_none, /* 0x3C -> 0x3F */
  op_rti,      op_eor_zpix, op_none,     op_none, /* 0x40 -> 0x43 */
  op_none,     op_eor_zp,   op_lsr_zp,   op_none, /* 0x44 -> 0x47 */
  op_pha,      op_eor_imm,  op_lsr_accu, op_none, /* 0x48 -> 0x4B */
  op_jmp_abs,  op_eor_abs,  op_lsr_abs,  op_none, /* 0x4C -> 0x4F */
  op_bvc,      op_eor_zpyi, op_none,     op_none, /* 0x50 -> 0x53 */
  op_none,     op_eor_zpx,  op_lsr_zpx,  op_none, /* 0x54 -> 0x57 */
  op_cli,      op_eor_absy, op_none,     op_none, /* 0x58 -> 0x5B */
  op_none,     op_eor_absx, op_lsr_absx, op_none, /* 0x5C -> 0x5F */
  op_rts,      op_adc_zpix, op_none,     op_none, /* 0x60 -> 0x63 */
  op_none,     op_adc_zp,   op_ror_zp,   op_none, /* 0x64 -> 0x67 */
  op_pla,      op_adc_imm,  op_ror_accu, op_none, /* 0x68 -> 0x6B */
  op_jmp_absi, op_adc_abs,  op_ror_abs,  op_none, /* 0x6C -> 0x6F */
  op_bvs,      op_adc_zpyi, op_none,     op_none, /* 0x70 -> 0x73 */
  op_none,     op_adc_zpx,  op_ror_zpx,  op_none, /* 0x74 -> 0x77 */
  op_sei,      op_adc_absy, op_none,     op_none, /* 0x78 -> 0x7B */
  op_none,     op_adc_absx, op_ror_absx, op_none, /* 0x7C -> 0x7F */
  op_none,     op_sta_zpix, op_none,     op_none, /* 0x80 -> 0x83 */
  op_sty_zp,   op_sta_zp,   op_stx_zp,   op_none, /* 0x84 -> 0x87 */
  op_dey,      op_none,     op_txa,      op_none, /* 0x88 -> 0x8B */
  op_sty_abs,  op_sta_abs,  op_stx_abs,  op_none, /* 0x8C -> 0x8F */
  op_bcc,      op_sta_zpyi, op_none,     op_none, /* 0x90 -> 0x93 */
  op_sty_zpx,  op_sta_zpx,  op_stx_zpy,  op_none, /* 0x94 -> 0x97 */
  op_tya,      op_sta_absy, op_txs,      op_none, /* 0x98 -> 0x9B */
  op_none,     op_sta_absx, op_none,     op_none, /* 0x9C -> 0x9F */
  op_ldy_imm,  op_lda_zpix, op_ldx_imm,  op_none, /* 0xA0 -> 0xA3 */
  op_ldy_zp,   op_lda_zp,   op_ldx_zp,   op_none, /* 0xA4 -> 0xA7 */
  op_tay,      op_lda_imm,  op_tax,      op_none, /* 0xA8 -> 0xAB */
  op_ldy_abs,  op_lda_abs,  op_ldx_abs,  op_none, /* 0xAC -> 0xAF */
  op_bcs,      op_lda_zpyi, op_none,     op_none, /* 0xB0 -> 0xB3 */
  op_ldy_zpx,  op_lda_zpx,  op_ldx_zpy,  op_none, /* 0xB4 -> 0xB7 */
  op_clv,      op_lda_absy, op_tsx,      op_none, /* 0xB8 -> 0xBB */
  op_ldy_absx, op_lda_absx, op_ldx_absy, op_none, /* 0xBC -> 0xBF */
  op_cpy_imm,  op_cmp_zpix, op_none,     op_none, /* 0xC0 -> 0xC3 */
  op_cpy_zp,   op_cmp_zp,   op_dec_zp,   op_none, /* 0xC4 -> 0xC7 */
  op_iny,      op_cmp_imm,  op_dex,      op_none, /* 0xC8 -> 0xCB */
  op_cpy_abs,  op_cmp_abs,  op_dec_abs,  op_none, /* 0xCC -> 0xCF */
  op_bne,      op_cmp_zpyi, op_none,     op_none, /* 0xD0 -> 0xD3 */
  op_none,     op_cmp_zpx,  op_dec_zpx,  op_none, /* 0xD4 -> 0xD7 */
  op_cld,      op_cmp_absy, op_none,     op_none, /* 0xD8 -> 0xDB */
  op_none,     op_cmp_absx, op_dec_absx, op_none, /* 0xDC -> 0xDF */
  op_cpx_imm,  op_sbc_zpix, op_none,     op_none, /* 0xE0 -> 0xE3 */
  op_cpx_zp,   op_sbc_zp,   op_inc_zp,   op_none, /* 0xE4 -> 0xE7 */
  op_inx,      op_sbc_imm,  op_nop,      op_none, /* 0xE8 -> 0xEB */
  op_cpx_abs,  op_sbc_abs,  op_inc_abs,  op_none, /* 0xEC -> 0xEF */
  op_beq,      op_sbc_zpyi, op_none,     op_none, /* 0xF0 -> 0xF3 */
  op_none,     op_sbc_zpx,  op_inc_zpx,  op_none, /* 0xF4 -> 0xF7 */
  op_sed,      op_sbc_absy, op_none,     op_none, /* 0xF8 -> 0xFB */
  op_none,     op_sbc_absx, op_inc_absx, op_none, /* 0xFC -> 0xFF */
};



void cpu_execute(cpu_t *cpu, mem_t *mem)
{
  uint8_t opcode;
  opcode = mem_read(mem, cpu->pc++);
  (opcode_function[opcode])(cpu, mem);
}



void cpu_reset(cpu_t *cpu, mem_t *mem)
{
  cpu->pc  = mem_read(mem, MEM_VECTOR_RESET_LOW);
  cpu->pc += mem_read(mem, MEM_VECTOR_RESET_HIGH) * 256;
  cpu->a = 0;
  cpu->x = 0;
  cpu->y = 0;
  cpu->sp = 0xFD;
  cpu->sr.n = 0;
  cpu->sr.v = 0;
  cpu->sr.b = 0;
  cpu->sr.d = 0;
  cpu->sr.i = 1;
  cpu->sr.z = 0;
  cpu->sr.c = 0;
}



void cpu_nmi(cpu_t *cpu, mem_t *mem)
{
  mem_write(mem, MEM_PAGE_STACK + cpu->sp--, cpu->pc / 256);
  mem_write(mem, MEM_PAGE_STACK + cpu->sp--, cpu->pc % 256);
  mem_write(mem, MEM_PAGE_STACK + cpu->sp--, cpu_status_get(cpu));
  cpu->sr.i = 1;
  cpu->pc  = mem_read(mem, MEM_VECTOR_NMI_LOW);
  cpu->pc += mem_read(mem, MEM_VECTOR_NMI_HIGH) * 256;
}



