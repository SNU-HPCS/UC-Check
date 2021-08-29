#ifndef SHARING_POLICY_CODEGEN_H
#define SHARING_POLICY_CODEGEN_H
#include "context.h"

extern const char *assem_snippet_multi_byte_nops[MULTI_BYTE_NOPS_MAX];
const char *get_multi_byte_nops(int nop_size);

int init_code_block(struct prog_context_t *context_ptr);

#endif //SHARING_POLICY_CODEGEN_H
