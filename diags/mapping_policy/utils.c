#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <assert.h>
#include <string.h>

#include "utils.h"

struct prog_context_t *init_context() {
    struct prog_context_t *context_ptr;
    if ((context_ptr = malloc(sizeof(struct prog_context_t))) == NULL) {
        return NULL;
    }
    memset(context_ptr, 0, sizeof(struct prog_context_t));

    return context_ptr;
}

void deinit_context(struct prog_context_t *context_ptr) {
    if (context_ptr->code_block) {
        munmap(context_ptr->code_block, context_ptr->code_block_size);
    }

    free(context_ptr);
}

static void print_usage(int argc, char* argv[]) {
    printf("Current Command\n");
    for (int i = 0; i < argc; ++i){
        printf("%s ", argv[i]);
    }
    printf("\n=== Usage\n"
           "%s [-t test_type] [-s set_size] [-w way_size] [-c code_chunk_size]\n"
           "\ttest_type: 1 (LINEAR) // 2 (COMPLEX)\n"
           "=== Help\n"
           "%s -h\n", argv[0], argv[0]);
}

int do_argparse(int argc, char *argv[], struct prog_context_t *context_ptr) {
    int opt;

    while ((opt = getopt(argc, argv, "t:s:w:c:")) != -1) {
        switch (opt) {
            case 't':
                context_ptr->test_type = atoi(optarg);
                if ((context_ptr->test_type <= TEST_TYPE_MIN) || (context_ptr->test_type >= TEST_TYPE_MAX)) {
                    printf("Invalid test_type: %d\n", context_ptr->test_type);
                    print_usage(argc, argv);
                    return -1;
                }
                break;
            case 's':
                context_ptr->set_size = atoi(optarg);
                if (context_ptr->set_size <= 0) {
                    printf("Invalid set_size: %d\n", context_ptr->set_size);
                    print_usage(argc, argv);
					return -1;
                }
                break;
            case 'w':
                context_ptr->way_size = atoi(optarg);
                if (context_ptr->way_size <= 0) {
                    printf("Invalid way_size: %d\n", context_ptr->way_size);
                    print_usage(argc, argv);
                    return -1;
                }
				break;
            case 'c':
                context_ptr->code_chunk_size = atoi(optarg);
                if (context_ptr->code_chunk_size <= 0) {
                    printf("Invalid code_chunk_size: %d\n", context_ptr->code_chunk_size);
                    print_usage(argc, argv);
                    return -1;
                }
                break;
            case '?':
                printf("unknown option: %c\n", optopt);
            default:
                print_usage(argc, argv);
                return -1;
        }
    }

    if (optind < argc) {
        for (; optind < argc; optind++) {
            printf("Unparased extra arguments: %s\n", argv[optind]);
        }
        print_usage(argc, argv);
        return -1;
    }

    if (context_ptr->test_type == 0 || context_ptr->set_size == 0 ||
        context_ptr->way_size == 0 || context_ptr->code_chunk_size == 0) {
        printf("Fail to init test_type (%d) & set_size (%d) & way_size (%d) & code_chunk_size (%d)\n",
               context_ptr->test_type, context_ptr->set_size, context_ptr->way_size, context_ptr->code_chunk_size);
        print_usage(argc, argv);
        return -1;
    }


    return 0;
}

void shuffle(int *array, size_t n) {
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

int dump_code_block(struct prog_context_t *context_ptr) {
    FILE *ptr;

    if ((ptr = fopen("code_block.bin", "wb")) == NULL) {
        return -1;
    }
    fwrite(context_ptr->code_block, context_ptr->code_block_size, 1, ptr);
    fclose(ptr);
    return 0;
}
