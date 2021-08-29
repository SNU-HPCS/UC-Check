#ifndef SHARING_POLICY_CONTEXT_H
#define SHARING_POLICY_CONTEXT_H

#define MULTI_BYTE_NOPS_MAX                 12
#define DEFAULT_CODE_CHUNK_SIZE             64
#define DEFAULT_NUM_NOP                     2

typedef enum {
    CHUNK_TYPE_MIN = 0,
    NOP_2_CHUNK,
    NOP_3_CHUNK,
    NOP_4_CHUNK,
    NOP_5_CHUNK,
    NOP_6_CHUNK,
	NOP_7_CHUNK,
	NOP_8_CHUNK,
	NOP_9_CHUNK,
	NOP_10_CHUNK,
	NOP_11_CHUNK,
    INT_CHUNK,
    CHUNK_TYPE_MAX,
} CHUNK_TYPE;

struct prog_context_t {
    CHUNK_TYPE chunk_type;          // [set by do_argparse]

    char *code_block;               // [set by init_code_block]
    int code_block_size;            // [set by do_argparse] should be aligned to code_chunk_size
    int code_chunk_size;            // [set by init_context]
    int code_block_num_chunks;      // [set by do_argparse]
    int num_nops_in_chunk;          // [set by do_argparse]
};

#endif //SHARING_POLICY_CONTEXT_H
