#ifndef MAPPING_POLICY_CONTEXT_H
#define MAPPING_POLICY_CONTEXT_H

typedef enum {
    TEST_TYPE_MIN = 0,
    TEST_TYPE_LINEAR = 1,
    TEST_TYPE_COMPLEX,
    TEST_TYPE_MAX,
} TEST_TYPE;

#define DEFAULT_CODE_BLOCK_SIZE             (1024*1024*64) // 64MB
#define DEFAULT_CODE_CHUNK_SIZE             (16)
struct prog_context_t {
    TEST_TYPE test_type;            // [set by do_argparse]

    int set_size;                   // [set by do_argparse]
    int way_size;                   // [set by do_argparse]
    int code_chunk_size;            // [set by do_argparse]

    char *code_block;
    int code_block_size;
    int num_code_chunks;
};

#endif //MAPPING_POLICY_CONTEXT_H
