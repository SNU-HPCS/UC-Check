#define _GNU_SOURCE
#include <asm/msr.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "./utils.h"

/**
 * Pin a thread to a core
 */
int pin_cpu(int cpu_num){
    int flag = 0;
    pid_t pid = getpid();
    cpu_set_t cpuset;
    
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_num, &cpuset);
    if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset)){
        fprintf(stderr, "[Error] scheduling affinity for arg: %d (pid: %d) failed\n", cpu_num, pid);
    }
    
    CPU_ZERO(&cpuset);
    if (sched_getaffinity(pid, sizeof(cpu_set_t), &cpuset)){
        fprintf(stderr, "[Error] retriving affinity for arg: %d (pid: %d) failed\n", cpu_num, pid);
    }
    
    if (CPU_ISSET(cpu_num, &cpuset)){
	    fprintf(stdout, "[Log] cpu affinity successfully set for arg: %d (pid: %d)\n", cpu_num, pid);
        flag = 1;
    }
    
    return flag;
}

/**
 * Vendor-specific settings
 * Should be run in both Victim and Attacker when starting
 */
void startup_setting() {
    int path_len;
    char cur_path[200];
    struct stat sb;
    int i;
    
    // Path 
    if(!getcwd(cur_path, sizeof(cur_path)))
        exit(0);

    for (i = strlen(cur_path) - 1; i >=0; i--) {
        if (cur_path[i] == '/')
            break;     
    }
    cur_path[i] = '\0';

    strcat(CUR_PATH, cur_path);
    strcat(RE_PATH, cur_path);
    strcat(RE_PATH, "/result");

    if (!(stat(RE_PATH, &sb) == 0 && S_ISDIR(sb.st_mode))) {
        if (mkdir(RE_PATH,0777) != 0) {
            printf("[Error] mkdir failed\n"); exit(1);
        }
    }

    fprintf(stdout, "[Log] [ setup info ]\n");
    fprintf(stdout, "[Log] ** cores : %d, %d\n", VICTIM_CORE, ATTACKER_CORE);
}

/**
 * Dump the result info .csv
 * If error, return -1
 */
int dump_result(struct Regs **result, uint64_t len_result, int warmup) {
    FILE *reFile; 
    char re_path[200], tmp1[100], tmp2[100];
    uint64_t val1, val2;
    int s,l,w;
   
    memset(re_path, 0, 200);
    strcat(re_path, RE_PATH);
    strcat(re_path, "/attack.csv"); 

    reFile = fopen(re_path, "w");
    
    if (reFile != NULL) {
        fputs("set,probe_idx,value\n", reFile);
        
        for (s = 0; s < UOP_SET; s++) {
            w = 0;
            for (l = 0; l < len_result; l++) {
                if ((l % len_result) <= warmup) {
                    w++; continue;    
                }
                val1 = (result[s][l]._edx1 << 32 | result[s][l]._eax1);
                val2 = (result[s][l]._edx2 << 32 | result[s][l]._eax2);
                val2 -= val1;
                
                memset(tmp1, 0, 100); sprintf(tmp1, "%d,", s); 
                memset(tmp2, 0, 100); sprintf(tmp2, "%d,", l-w);
                strcat(tmp1, tmp2); 
                memset(tmp2, 0, 100); sprintf(tmp2, "%ld\n", val2);
                strcat(tmp1, tmp2); 
                fputs(tmp1, reFile);
            } 
        }
        fclose(reFile);
        return 0;
    }
    else {
        fprintf(stdout, "[Error] opening the result file failed\n"); 
        return -1; 
    }
}
