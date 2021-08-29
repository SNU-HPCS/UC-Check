#ifndef SHARING_POLICY_UTILS_H
#define SHARING_POLICY_UTILS_H
#include <stdlib.h>

#include "context.h"

struct prog_context_t *init_context();
void deinit_context(struct prog_context_t *context_ptr);
int do_argparse(int argc, char *argv[], struct prog_context_t *context_ptr);
void shuffle(int *array, size_t n);
int dump_code_block(struct prog_context_t *context_ptr);

#endif //SHARING_POLICY_UTILS_H
