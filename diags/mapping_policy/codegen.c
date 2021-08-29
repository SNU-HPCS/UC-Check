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
};

const char *get_multi_byte_nops(int nop_size) {
    if (nop_size <= 0 || nop_size > MULTI_BYTE_NOPS_MAX) {
        return NULL;
    } else {
        return assem_snippet_multi_byte_nops[nop_size - 1];
    }
}

static char *gen_code_chunk(int cur_idx, int next_idx, struct prog_context_t *context_ptr) {
    const int code_chunk_size = context_ptr->code_chunk_size;
    const int code_stride_size = context_ptr->code_chunk_size * context_ptr->set_size;
    int i;
    int base_inst_size, num_base_inst;
    int code_chunk_target_size;
    int32_t cur_ip_offset, next_ip_offset, ip_offset_diff;
    char *code_chunk = malloc(code_chunk_size);
    if (code_chunk == NULL) {
        return NULL;
    }
    memset(code_chunk, '\x90', code_chunk_size);

    base_inst_size = 2; // 2-byte nops
    num_base_inst = 5;  // 5 instruction
    code_chunk_target_size = base_inst_size * num_base_inst;

    // init a code chunk
    for (i = 0; i < num_base_inst; i++) {
        memcpy(code_chunk + i * base_inst_size, get_multi_byte_nops(base_inst_size), base_inst_size);
    }

    // Add JMP to the next code chunk (next_idx)
    cur_ip_offset = cur_idx * code_stride_size + code_chunk_target_size + 5; // 5 bytes JMP instruction
    next_ip_offset = next_idx * code_stride_size;
    ip_offset_diff = next_ip_offset - cur_ip_offset;
    code_chunk[code_chunk_target_size] = '\xE9';
    memcpy(code_chunk + code_chunk_target_size + 1, &ip_offset_diff, sizeof(int32_t));

    return code_chunk;
}

int init_code_block(struct prog_context_t *context_ptr) {
    context_ptr->num_code_chunks = context_ptr->way_size + 1;

    // calculate & allocate code_block_size
    context_ptr->code_block_size = context_ptr->num_code_chunks * context_ptr->set_size * context_ptr->code_chunk_size;
    if ((context_ptr->code_block = mmap(NULL, context_ptr->code_block_size,
                                        PROT_EXEC | PROT_READ | PROT_WRITE,
                                        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
        perror("MMAP failed");
        return -1;
    }
    memset(context_ptr->code_block, '\x90', context_ptr->code_block_size);

    for (int i = 0; i < context_ptr->num_code_chunks; i++) {
        int cur_idx, next_idx;
        char *code_chunk;
        cur_idx = i % context_ptr->num_code_chunks;
        next_idx = (i+1) % context_ptr->num_code_chunks;
        code_chunk = gen_code_chunk(cur_idx, next_idx, context_ptr);
        memcpy(context_ptr->code_block + (context_ptr->code_chunk_size * context_ptr->set_size) * cur_idx,
               code_chunk, context_ptr->code_chunk_size);
        free(code_chunk);
    }

    return 0;
}
