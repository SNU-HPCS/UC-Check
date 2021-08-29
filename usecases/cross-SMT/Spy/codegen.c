#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <assert.h>
#include <sys/mman.h>

#include "../global.h"
#include "codegen.h"

#define TOTAL_SET           (0)
#define SINGLE_SET          (1)

const char *assem_snippet_multi_byte_nops[MULTI_BYTE_NOPS_MAX] = {
        "\x90",
        "\x66\x90",
        "\x0F\x1F\x00",
        "\x0F\x1F\x40\x00",
        "\x0F\x1F\x44\x00\x00",
        "\x66\x0F\x1F\x44\x00\x00",
        "\x0F\x1F\x80\x00\x00\x00\x00",
        "\x0F\x1F\x84\x00\x00\x00\x00\x00",
        "\x66\x0F\x1F\x84\x00\x00\x00\x00\x00",
        "\x66\x66\x0F\x1F\x84\x00\x00\x00\x00\x00",     
        "\x66\x66\x66\x0F\x1F\x84\x00\x00\x00\x00\x00",   
};

static const char *get_multi_byte_nops(int nop_size) {
    if (nop_size <= 0 || nop_size > MULTI_BYTE_NOPS_MAX) {
        return NULL;
    } else {
        return assem_snippet_multi_byte_nops[nop_size - 1];
    }
}

static void shuffle(int *array, size_t n) {
    if (n > 1)
    {
        size_t i;
        for (i = 0; i < n - 1; i++)
        {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

static int *get_random_chain(int size) {
    int i;
    int *chain;
    if (size == 0) {
        return NULL;
    }

    chain = malloc(sizeof(int) * size);
    for (i = 0; i < size; i++) {
        chain[i] = i;
    }
    //shuffle(chain, size);
    
    return chain;
}

/**
 * Make an unit chunk 
 * If single is true, only the chunks in a single set is formed into a chunk chain
 * If next_idx is -1, fill the end of the chunk with ret (not jmp) 
 */
static char *gen_code_chunk(int cur_idx, int next_idx, struct prog_context_t *context_ptr, int single) {
    const int code_chunk_size = context_ptr->code_chunk_size;
    const int code_chunk_target_size;
    const int set = context_ptr->num_set; 
    
    int i;
    int end_size;       // 5 (jmp) or 1 (ret)
    int base_inst_size, num_base_inst, remaining_bytes;
    int32_t cur_ip_offset, next_ip_offset, ip_offset_diff;
    char *code_chunk = malloc(code_chunk_size);
    if (code_chunk == NULL) {
        return NULL;
    }

    if ((context_ptr->chunk_type == NOP_2_CHUNK) ||
        (context_ptr->chunk_type == NOP_3_CHUNK) ||
        (context_ptr->chunk_type == NOP_4_CHUNK) ||
        (context_ptr->chunk_type == NOP_5_CHUNK) ||
        (context_ptr->chunk_type == NOP_6_CHUNK) ||
        (context_ptr->chunk_type == NOP_7_CHUNK) ||
        (context_ptr->chunk_type == NOP_8_CHUNK) ||
        (context_ptr->chunk_type == NOP_9_CHUNK) ||
        (context_ptr->chunk_type == NOP_10_CHUNK)||
        (context_ptr->chunk_type == NOP_11_CHUNK)) {
        
        if (context_ptr->chunk_type == NOP_2_CHUNK) {
            base_inst_size = 2;
        } else if (context_ptr->chunk_type == NOP_3_CHUNK) {
            base_inst_size = 3;
        } else if (context_ptr->chunk_type == NOP_4_CHUNK) {
            base_inst_size = 4;
        } else if (context_ptr->chunk_type == NOP_5_CHUNK) {
            base_inst_size = 5;
        } else if (context_ptr->chunk_type == NOP_6_CHUNK) {
            base_inst_size = 6;
        } else if (context_ptr->chunk_type == NOP_7_CHUNK) {
            base_inst_size = 7;
        } else if (context_ptr->chunk_type == NOP_8_CHUNK) {
            base_inst_size = 8;
        } else if (context_ptr->chunk_type == NOP_9_CHUNK) {
            base_inst_size = 9;
        } else if (context_ptr->chunk_type == NOP_10_CHUNK) {
            base_inst_size = 10;
        } else if (context_ptr->chunk_type == NOP_11_CHUNK) {
            base_inst_size = 11;
        } else {
            assert(0);
        }
        
        // ** NOTE: manually configure X (where X is #of nops) 
        // Let a line contains X n-byte Nops and 1 Jmp 
        const int code_chunk_target_size = 6 * base_inst_size;

        num_base_inst = code_chunk_target_size / base_inst_size;
        remaining_bytes = code_chunk_target_size % base_inst_size;

        // init a code chunk
        for (i = 0; i < num_base_inst; i++) {
            memcpy(code_chunk + i * base_inst_size, get_multi_byte_nops(base_inst_size), base_inst_size);
        }
        memcpy(code_chunk + i * base_inst_size, get_multi_byte_nops(remaining_bytes), remaining_bytes);

        
        if (single == SINGLE_SET) { 
            cur_ip_offset = cur_idx * (set * code_chunk_size) + code_chunk_target_size + 5; // 5 bytes JMP
            next_ip_offset = next_idx * (set * code_chunk_size);
        }
        else if (single == TOTAL_SET) { 
            cur_ip_offset = cur_idx * code_chunk_size + code_chunk_target_size + 5; // 5 bytes JMP
            next_ip_offset = next_idx * code_chunk_size;
        }
        else
            assert(0);

        ip_offset_diff = next_ip_offset - cur_ip_offset;
      
        // Non-last chunk (jmp) 
        if (next_idx != -1) { 
            end_size = 5;
            code_chunk[code_chunk_target_size] = '\xE9';
            memcpy(code_chunk + code_chunk_target_size + 1, &ip_offset_diff, sizeof(int32_t));
        }
        // Last chunk (ret)
        else {
            end_size = 1;
            code_chunk[code_chunk_target_size] = '\xC3';
        }

		int t_nopsize = 1;
		for (i = code_chunk_target_size + end_size; i + t_nopsize < code_chunk_size; i += t_nopsize) {
			memcpy(code_chunk + i, get_multi_byte_nops(t_nopsize), t_nopsize); 
		}
		if (i < code_chunk_size) {
			memcpy(code_chunk + i, get_multi_byte_nops(code_chunk_size - i), code_chunk_size - i);
			i = code_chunk_size;
		}
    }

    return code_chunk;
}

/**
 * Make a loop of uop chunks with a specifc working-set size.
 * If nu_set is not -1, only the chunks in a single set (@nu_set) is formed into a loop.
 * Returns the offset of the allocated block.
 */
static int _init_code_block(struct prog_context_t *context_ptr, int nu_set) {
    int *random_chain;
    int chain_size;
    int tmp;
    const int set = context_ptr->num_set; 

    if (nu_set == -1)
        chain_size = context_ptr->code_block_num_chunks;
    else if (nu_set >= 0)
        chain_size = context_ptr->code_block_num_chunks / set;
    else
        return -1;
         
    if ((context_ptr->code_block_num_chunks % set) != 0) {
        return -1;
    }

    // Get random chain 
    if ((random_chain = get_random_chain(chain_size)) == NULL)
        return -1;
   
    // Form a chain 
    int cur_idx, next_idx, offset;
    char *code_chunk;
    for (int i = 1; i <= chain_size; i++) {
        if (i < chain_size) {
            cur_idx = random_chain[i-1];
            next_idx = random_chain[i];
        } else {
            cur_idx = random_chain[i-1];
            next_idx = -1;
        }
    
        // Generate a chunk 
        if (nu_set == -1) {
            // Set Single OFF for gen_code_chunk 
            if ((code_chunk = gen_code_chunk(cur_idx, next_idx, context_ptr, \
                                             TOTAL_SET)) == NULL)
                return -1;
            
            offset = context_ptr->code_chunk_size * cur_idx;
        }   
        else {
            // Set Single ON for gen_code_chunk 
            // Set code_chunk at base + "offset-for-the-set" + chunk-idx
            if ((code_chunk = gen_code_chunk(cur_idx, next_idx, context_ptr, \
                                             SINGLE_SET)) == NULL)
                return -1;

            offset = context_ptr->code_chunk_size * (cur_idx*set + nu_set);
        }
        
        memcpy(context_ptr->code_block + offset, code_chunk, context_ptr->code_chunk_size);
        free(code_chunk);
    }
    
    // Set the start pointer of the code block
    tmp = (nu_set == -1 ? random_chain[0] : (random_chain[0]*set + nu_set)); 
    tmp = context_ptr->code_chunk_size * tmp;

    if (nu_set == -1)
        context_ptr->start[0] = context_ptr->code_block + tmp;
    else {
        context_ptr->start[nu_set] = context_ptr->code_block + tmp;
    } 
    free(random_chain);
    return tmp;
}

/**
 * Main util function.
 * If all the parameters in prog_context_t are set properly,
 * Generate a code block(s)
 * 
 * If SET_WISE == SET_WISE_OFF:
 *  - Generates a chain of chunks all-together (head ptr @start[0])
 * If SET_WISE == SET_WISE_ON:
 *  - Generates a chain of chunks set-by-set (head ptrs @start[set number])
 */
int init_code_block(struct prog_context_t *context_ptr, int SET_WISE) {
    int i;
    int set;
    int cb_size; 
    int cb_num_chunks;
    int offset;
    
    set = context_ptr->num_set; 
    cb_size = context_ptr->code_chunk_size;
    cb_num_chunks = context_ptr->code_block_num_chunks;

    assert(set > 0);
    assert(cb_size % 8 == 0);
    assert(cb_num_chunks > 0);
   
    // Allocate the code block area 
    if ((context_ptr->code_block = mmap(NULL, (cb_size * cb_num_chunks), \
                                        PROT_EXEC | PROT_READ | PROT_WRITE, \
                                        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
        fprintf(stdout, "[Error] mmap() failed in init_code_block()");
        return -1;
    }
    
    context_ptr->start = (char **)malloc(set * sizeof(char *));
    context_ptr->offsets = (int *)malloc(set * sizeof(int));

    // Generate code chunks
    if (SET_WISE == SET_WISE_OFF) {
        if ((offset = _init_code_block(context_ptr, -1)) == -1) {
            return -1; 
        }
        return 0; 
    } else if (SET_WISE == SET_WISE_ON) {
       for (i = 0; i < set; i++) {
           if ((offset = _init_code_block(context_ptr, i)) == -1)
               return -1;
           context_ptr->offsets[i] = offset; 
       }
       return 0; 
    } else {
        return -1;        
    }
}

void cleanup_code_block(struct prog_context_t *context_ptr) {
    munmap(context_ptr->code_block, \
           (context_ptr->code_chunk_size * context_ptr->code_block_num_chunks));
    free(context_ptr->start);
    free(context_ptr->offsets);
    free(context_ptr);    
}

int dump_code_block(struct prog_context_t *context_ptr, char note[20]) {
    FILE *ptr;
    char off_path[200];
    char cb_path[200];
    char tmp[200];
    int i;

    memset(off_path, 0, 200);
    memset(cb_path, 0, 200);
    memset(tmp, 0, 200);
     
    strcat(off_path, RE_PATH);
    strcat(off_path, "/offset.txt");
    strcat(cb_path, RE_PATH);
    strcat(cb_path, "/code_block.bin");
    
    if ((ptr = fopen(off_path, "wb")) == NULL) {
        printf("[Error] opening offset file failed\n");
        return -1;
    }
    
    memset(tmp, 0, sizeof(tmp));
    strcat(tmp, note); fputs(tmp, ptr); 
    
    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "code block: 0x%lx\n", (uint64_t)context_ptr->code_block); fputs(tmp, ptr); 
    
    for (i = 0; i < context_ptr->num_set; i++) {
        memset(tmp, 0, sizeof(tmp));
        sprintf(tmp, "offset (%d) 0x%x\n", i, context_ptr->offsets[i]);
        fputs(tmp, ptr); 
    } 
    
    fclose(ptr);

    if ((ptr = fopen(cb_path, "wb")) == NULL) {
        printf("[Error] opening code-block file failed\n");
        return -1;
    }
    fwrite(context_ptr->code_block, \
    (context_ptr->code_chunk_size * context_ptr->code_block_num_chunks), 1, ptr);
    fclose(ptr);
    return 0;
}
