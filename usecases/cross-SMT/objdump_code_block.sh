#!/bin/bash
objdump -b binary -m i386:x86-64 -D result/code_block.bin > result/cb_objdump.txt 
objdump -d Spy/spy > result/spy_objdump.txt 
objdump -d Victim/victim > result/victim_objdump.txt 
