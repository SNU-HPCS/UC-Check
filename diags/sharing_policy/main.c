#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"
#include "context.h"
#include "codegen.h"

int main(int argc, char *argv[]) {
    int rc;
    struct prog_context_t *prog_context = NULL;
    void (*func)(void);

    srand((unsigned int)time(NULL));

    if ((prog_context = init_context()) == NULL) {
        fprintf(stderr, "Fail to init context\n");
        rc = -1;
        goto err;
    }

    if ((rc = do_argparse(argc, argv, prog_context)) != 0) {
        fprintf(stderr, "Fail to parse arguments\n");
        goto err;
    }

    if ((rc = init_code_block(prog_context)) != 0) {
        fprintf(stderr, "Fail to init an executable code block\n");
        goto err;
    }

    if ((rc = dump_code_block(prog_context)) != 0) {
        fprintf(stderr, "Fail to dump code_block\n");
        goto err;
    }

    func = (void*)prog_context->code_block;
    func();

    deinit_context(prog_context);
    return 0;
err:
    if (prog_context) deinit_context(prog_context);
    return rc;
}
