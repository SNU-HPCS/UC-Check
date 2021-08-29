#ifndef __CODEGEN_H__
#define __CODEGEN_H__

#define CODE_CHUNK_SIZE_32                  (32)
#define CODE_CHUNK_SIZE_64                  (64)
#define MULTI_BYTE_NOPS_MAX                 (11)

#define SET_WISE_OFF                        (0)
#define SET_WISE_ON                         (1) 

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
    CHUNK_TYPE chunk_type;        
    char *code_block;
    char **start;
    int code_chunk_size;          
    int code_block_num_chunks;
    int num_set;

    // For debugging
    int *offsets;
};

int init_code_block(struct prog_context_t *context_ptr, int SET_WISE);
void cleanup_code_block(struct prog_context_t *context_ptr);
int dump_code_block(struct prog_context_t *context_ptr, char note[20]);

#endif
