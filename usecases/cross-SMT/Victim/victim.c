/**
 * Victim program
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "../global.h"
#include "../utils.h"

// You may configure the gap between each encrypt()
// and the length of func_A() and func_B()
#define BUSY_WAIT           0
#define FUNC_CNT            200

void func_A() {
    int i, count;
    for (i = 0; i < FUNC_CNT; i++)
        count++;
}

void func_B() {
    int i, count;
    for (i = 0; i < FUNC_CNT; i++)
        count++;
}

/**
 * Main part of victim.c
 * It takes different path depending on the key value bit (func_A vs. func_B)
 */
void encrypt(char *key) {
    int i;
    for (i = 0; i < (KEYLEN); i++) {
        if (key[i] == '1')
            func_A();
        else
            func_B(); 
    }
}

int main(int argc, char **argv) {
    int c;
    bool flag = false;
    char key[KEYLEN + 1];

    /* Retrieve a key */
    while ((c = getopt(argc, argv, "k:")) != -1) {
        switch(c) {
            case 'k':
                memcpy(key, optarg, KEYLEN+1);
                if (strlen(key) != KEYLEN)
                    break;
                flag = true; break;
            
            case '?':
                break;
        }     
    }

    if (!flag) {
        printf("[Usage] Option -k requires a %d-character key to encrypt...\n", KEYLEN);
        return 0;     
    }

    /* Pin the process */
    if (!pin_cpu(VICTIM_CORE)) {
        perror("[Error] pinning the victim process failed\n");
        return 0;     
    }

    /* Victim runs */
    while (1) {
        encrypt(key);
        busy_wait(BUSY_WAIT);
    } 
    
    return 0;
}
