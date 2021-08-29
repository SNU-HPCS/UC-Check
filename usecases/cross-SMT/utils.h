#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <stdbool.h>
#include "./global.h"

#define busy_wait(cycles)   for(int i = 0; i < cycles; i++)\
                                __asm__ __volatile__ ("nop");

#define NOP1    __asm__ __volatile__ ("nop":::);
#define NOP4    NOP1 NOP1 NOP1 NOP1
#define NOP8    NOP4 NOP4
#define NOP16   NOP8 NOP8
#define NOP32   NOP16 NOP16
#define NOP64   NOP32 NOP32 
#define NOP128  NOP64 NOP64
#define NOP256  NOP128 NOP128

/* Special structure for saving edx and eax form a single 64bit value */
struct Regs {
    uint64_t _edx1;
    uint64_t _eax1;
    uint64_t _edx2;
    uint64_t _eax2;
};

int pin_cpu(int cpu_num);
void startup_setting();
int dump_result(struct Regs **result, uint64_t len_result, int warmup);

#endif
