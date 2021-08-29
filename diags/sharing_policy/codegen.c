#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "utils.h"
#include "codegen.h"


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

const int intop_code_len = 20;
const char *assem_snippet_intops =
    // Dependent int
     "\x66\x45\x01\xc0" 
     "\x66\x45\x01\xc0" 
     "\x66\x45\x01\xc0" 
     "\x66\x45\x01\xc0"
     "\x66\x45\x01\xc0";
//    // Independent int
//    "\x66\x45\x01\xc1"
//    "\x66\x45\x01\xc2"
//    "\x66\x45\x01\xc3"
//    "\x66\x45\x01\xc4"
//    "\x66\x45\x01\xc5";
	

const char *get_multi_byte_nops(int nop_size) {
    if (nop_size <= 0 || nop_size > MULTI_BYTE_NOPS_MAX) {
        return NULL;
    } else {
        return assem_snippet_multi_byte_nops[nop_size - 1];
    }
}

static int *get_random_chain(int size) {
    int i;
    int *chain;
    int left;
    if (size == 0) {
        return NULL;
    }

    chain = malloc(sizeof(int) * size);
    for (i = 0; i < size; i++) {
        chain[i] = i;
    }
    
    // Block-wise shuffle
    for (i = 0; i < (size / 32); i++) {
        shuffle(chain + 32 * i, 32);     
    }
    
    left = size - i * 32;
    if (left != 0) {
        shuffle(chain + 32 * i, left);     
    }

    return chain;
}

static char *gen_code_chunk(int cur_idx, int next_idx, struct prog_context_t *context_ptr) {
    const int code_chunk_size = context_ptr->code_chunk_size;
    const int code_chunk_target_size;
    
    int i;
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
        
        // Let a line contains X n-byte Nops and 1 Jmp 
        const int code_chunk_target_size = context_ptr->num_nops_in_chunk * base_inst_size;
       
        num_base_inst = code_chunk_target_size / base_inst_size;
        remaining_bytes = code_chunk_target_size % base_inst_size;

        // init a code chunk
        for (i = 0; i < num_base_inst; i++) {
            memcpy(code_chunk + i * base_inst_size, get_multi_byte_nops(base_inst_size), base_inst_size);
        }
        memcpy(code_chunk + i * base_inst_size, get_multi_byte_nops(remaining_bytes), remaining_bytes);

        // Add JMP to the next code chunk (next_idx)
        cur_ip_offset = cur_idx * code_chunk_size + code_chunk_target_size + 5; // 5 bytes JMP instruction
        next_ip_offset = next_idx * code_chunk_size;
        ip_offset_diff = next_ip_offset - cur_ip_offset;
        code_chunk[code_chunk_target_size] = '\xE9';
        memcpy(code_chunk + code_chunk_target_size + 1, &ip_offset_diff, sizeof(int32_t));

		int t_nopsize = 1;
		for (i = code_chunk_target_size + 5; i + t_nopsize < code_chunk_size; i += t_nopsize) {
			memcpy(code_chunk + i, get_multi_byte_nops(t_nopsize), t_nopsize); 
		}
		if (i < code_chunk_size) {
			memcpy(code_chunk + i, get_multi_byte_nops(code_chunk_size - i), code_chunk_size - i);
			i = code_chunk_size;
		}
    } else if (context_ptr->chunk_type == INT_CHUNK) {
		// init a code chunk
		memcpy(code_chunk, assem_snippet_intops, intop_code_len);
		// add JMP to the next code chunk
        cur_ip_offset = cur_idx * code_chunk_size + intop_code_len + 5; // 5 bytes JMP instruction
        next_ip_offset = next_idx * code_chunk_size;
        ip_offset_diff = next_ip_offset - cur_ip_offset;
        code_chunk[intop_code_len] = '\xE9';
        memcpy(code_chunk + intop_code_len + 1, &ip_offset_diff, sizeof(int32_t));

		int t_nopsize = 2;
		for (i = intop_code_len + 5; i + t_nopsize < code_chunk_size; i += t_nopsize) {
			memcpy(code_chunk + i, get_multi_byte_nops(t_nopsize), t_nopsize); 
		}
		if (i < code_chunk_size) {
			memcpy(code_chunk + i, get_multi_byte_nops(code_chunk_size - i), code_chunk_size - i);
			i = code_chunk_size;
		}
    }

    return code_chunk;
}

int init_code_block(struct prog_context_t *context_ptr) {
    int *random_chain;

    if ((context_ptr->code_block = mmap(NULL, context_ptr->code_block_size,
                                        PROT_EXEC | PROT_READ | PROT_WRITE,
                                        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
        perror("MMAP failed");
        return -1;
    }

    if ((random_chain = get_random_chain(context_ptr->code_block_num_chunks)) == NULL) {
        return -1;
    }

    for (int i = 1; i <= context_ptr->code_block_num_chunks; i++) {
        int cur_idx, next_idx;
        if (i < context_ptr->code_block_num_chunks) {
            cur_idx = random_chain[i-1];
            next_idx = random_chain[i];
        } else {
            cur_idx = random_chain[i-1];
            next_idx = random_chain[0];
        }

        char *code_chunk = gen_code_chunk(cur_idx, next_idx, context_ptr);

        memcpy(context_ptr->code_block + context_ptr->code_chunk_size * cur_idx,
                code_chunk, context_ptr->code_chunk_size);
        free(code_chunk);
    }

    free(random_chain);

    return 0;
}
