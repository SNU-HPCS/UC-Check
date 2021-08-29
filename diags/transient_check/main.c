#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#ifdef _MSC_VER
#include <intrin.h>        /* for rdtscp and clflush */
#pragma optimize("gt",on)
#else
#include <x86intrin.h>     /* for rdtscp and clflush */
#include <sys/mman.h>

#endif

#include "nanoBench.h"


/*
 * all bytes are set as '\x90' except for few bytes
 * --------------------
 * | Noraml region    |
 * | (4096 bytes)     |
 * |-------------------
 * | Dummy region     |
 * | (4096 bytes)     |
 * |-------------------
 * | never-reach zone |
 * | (4096 bytes)     |
 * |-------------------
 * | transient region |
 * | (4096 bytes)     |
 * --------------------
 */
#define CB_TOTAL_SIZE       16384
#define CB_NORMAL_START     0
#define CB_DUMMY_START      4096
#define CB_NEVER_START      (CB_DUMMY_START + 4096)
#define CB_TRANSIENT_START  (CB_NEVER_START + 4096)

char *code_block;
uint8_t unused1[64];
unsigned int code_normal_threshold = CB_DUMMY_START;

/********************************************************************
For nanoBench
********************************************************************/
int pin_cpu(int cpu_num){
    int flag = 0;
    pid_t pid = getpid();
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(cpu_num, &cpuset);
    if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset)){
        fprintf(stderr, "[Error] scheduling affinity for arg: %d (pid: %d) failed\n", cpu_num, pid);
    }
    CPU_ZERO(&cpuset);
    if (sched_getaffinity(pid, sizeof(cpu_set_t), &cpuset)){
        fprintf(stderr, "[Error] retriving affinity for arg: %d (pid: %d) failed\n", cpu_num, pid);
    }
    if (CPU_ISSET(cpu_num, &cpuset)){
        fprintf(stdout, "[Log] cpu affinity successfully set for arg: %d (pid: %d)\n", cpu_num, pid);
        flag = 1;
    }
    return flag;
}

int nanoBench_init() {
    int i;

    /* Pin cpu */
    int target_cpuid = 1;
    if (!pin_cpu(target_cpuid)) {
        fprintf(stdout, "[Error] pinning the spy process failed\n");
        return -1;
    }
    cpu = target_cpuid;

    if (check_cpuid()) {
        fprintf(stdout, "[Error] checking cpu-id failed\n");
        return 1;
    }

    char *target_events;
    if (is_Intel_CPU) {
        target_events = "79.04 IDQ.MITE_UOPS\n"
                        "79.08 IDQ.DSB_UOPS";
    } else {
        target_events = "0AA.01 DeDisUopsFromDecoder.DecoderDispatched\n"
                        "0AA.02 DeDisUopsFromDecoder.OpCacheDispatched";
    }
    pfc_config_file_content = calloc(strlen(target_events)+1, sizeof(char));
    memcpy(pfc_config_file_content, target_events, strlen(target_events));
    parse_counter_configs();

    for (i = 0; i < n_pfc_configs; i += n_programmable_counters) {
        int end = i + n_programmable_counters;
        if (end > n_pfc_configs) {
            end = n_pfc_configs;
        }
        configure_perf_ctrs_programmable(i, end, 1, 0);  // configure
    }
}

/********************************************************************
Probe function
********************************************************************/
static void __attribute__ ((noinline)) probe(void (*func)(void), uint64_t *results_before, uint64_t *results_after) {
    uint64_t b0, b1, a0, a1;
    __asm__ __volatile__(
            "mov $0, %%rcx                           \n"
            "lfence; rdpmc; lfence                   \n"
            "shl $32, %%rdx; or %%rdx, %%rax"
            : "=a" (b0): : );
    __asm__ __volatile__(
            "mov $1, %%rcx                           \n"
            "lfence; rdpmc; lfence                   \n"
            "shl $32, %%rdx; or %%rdx, %%rax"
            : "=a" (b1): : );

    func();

    __asm__ __volatile__(
            "mov $0, %%rcx                           \n"
            "lfence; rdpmc; lfence                   \n"
            "shl $32, %%rdx; or %%rdx, %%rax"
            : "=a" (a0): : );
    __asm__ __volatile__(
            "mov $1, %%rcx                           \n"
            "lfence; rdpmc; lfence                   \n"
            "shl $32, %%rdx; or %%rdx, %%rax"
            : "=a" (a1): : );
    results_before[0] = b0;
    results_before[1] = b1;
    results_after[0] = a0;
    results_after[1] = a1;
}

/********************************************************************
Target functions
********************************************************************/
uint8_t temp = 0;  /* Used so compiler won't optimize out checker() */
void __attribute__ ((noinline)) checker (size_t x) {
    if (x < code_normal_threshold) {
        temp &= ((int (*)(void))(code_block + x))();
    }
}

/********************************************************************
Analysis code
********************************************************************/

int do_check() {
    int i, j, tries;
    uint64_t result_before[2] = {0,0}, result_after[2] = {0,0};
    uint64_t num_uop_cache = 0, num_uop_decoder = 0;
    size_t transient_code_idx = CB_TRANSIENT_START;
    size_t normal_code_idx = CB_NORMAL_START;
    size_t x;

    for (tries = 999; tries > 0; tries--) {
        for (i = 0; i < CB_TOTAL_SIZE; i += 64) {
            _mm_clflush(&code_block[i]);    // Flush all code_blocks
        }

        /* 30 loops: 5 training runs (x=training_x) per attack run (x=malicious_x) */
        for (j = 30; j >= 0; j--) {
            _mm_clflush(&code_normal_threshold);
            for (volatile int z = 0; z < 100; z++) {}  /* Delay */

            /* Bit twiddling to set x=training_x if j%6!=0 or malicious_x if j%6==0 */
            /* Avoid jumps in case those tip off the branch predictor */
            x = ((j % 6) - 1) & ~0xFFFF;   /* Set x=FFF.FF0000 if j%6==0, else x=0 */
            x = (x | (x >> 16));           /* Set x=-1 if j&6=0, else x=0 */
            x = normal_code_idx ^ (x & (transient_code_idx ^ normal_code_idx));

            /* Call the victim! */
            checker(x);
        }

        for (volatile int z = 0; z < 100; z++) {}  /* Delay */
        /* Read performance counters while execution function foo */
#ifdef DEBUG_MODE
        /* For debugging, accessing to the never reached region makes most uops decoded from the decoder not uop cache  */
        probe((void *) (code_block + CB_NEVER_START), result_before, result_after);
#else /* DEBUG_MODE */
        probe((void *) (code_block + CB_TRANSIENT_START), result_before, result_after);
#endif
        num_uop_decoder += ((result_after[0] > result_before[0]) ? result_after[0] - result_before[0] : 0);
        num_uop_cache += ((result_after[1] > result_before[1]) ? result_after[1] - result_before[1] : 0);
    }

    printf("=== Summary ===\n");
    printf("  uops from decoder  : %lu\n", num_uop_decoder);
    printf("  uops from uopcache : %lu\n", num_uop_cache);
}

#define NOP_JMPRET_BLOCK_SIZE 16
char *nop_jmp_block = "\x0f\x1f\x40\x00"    // 4-byte nop
                      "\x0f\x1f\x40\x00"    // 4-byte nop
                      "\x0f\x1f\x40\x00"    // 4-byte nop
                      "\x90\x90\xeb\x30";   // nop / nop / JMP next chunk
char *nop_ret_block = "\x0f\x1f\x40\x00"    // 4-byte nop
                      "\x0f\x1f\x40\x00"    // 4-byte nop
                      "\x0f\x1f\x40\x00"    // 4-byte nop
                      "\x90\x90\xc3\x90";   // nop / nop / RET

void init_code_block(char *codechk) {
    memcpy(codechk, nop_jmp_block, NOP_JMPRET_BLOCK_SIZE);
    memcpy(codechk + 64, nop_jmp_block, NOP_JMPRET_BLOCK_SIZE);
    memcpy(codechk + (64 * 2), nop_jmp_block, NOP_JMPRET_BLOCK_SIZE);
    memcpy(codechk + (64 * 3), nop_ret_block, NOP_JMPRET_BLOCK_SIZE);
}

int main(int argc, const char **argv) {
	/* Initialize nanoBench */
    nanoBench_init();

    /* Initialize code_block */
    if ((code_block = mmap(NULL, CB_TOTAL_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
        perror("MMAP failed");
        return -1;
    }
    memset(code_block, '\x90', CB_TOTAL_SIZE);
    init_code_block(&code_block[CB_NORMAL_START]);
    init_code_block(&code_block[CB_DUMMY_START]);
    init_code_block(&code_block[CB_TRANSIENT_START]);
    init_code_block(&code_block[CB_NEVER_START]);

	//////////////////////////////
	////// main measurement //////
	//////////////////////////////
	do_check();

	/* Dump code block */
    FILE *ptr;
    if ((ptr = fopen("code_block.bin", "wb")) == NULL) {
        return -1;
    }
    fwrite(code_block, CB_TOTAL_SIZE, 1, ptr);
    fclose(ptr);
    if (munmap(code_block, CB_TOTAL_SIZE) != 0) {
        perror("MUNMAP failed");
        return -1;
    }
    return (0);
}
