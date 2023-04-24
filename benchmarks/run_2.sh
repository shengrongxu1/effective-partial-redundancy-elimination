#!/bin/bash
### run.sh
### Benchmark Runner Script
### Locate this script at each benchmark directory. e.g, 583simple/run.sh
### Usage: sh run.sh ${benchmark_name} ${input} 
### Usage: sh run.sh compress compress.in OR sh run.sh simple OR sh run.sh wc cccp.c
### Note: Do NOT include inputs/ in ${input}, `./run.sh compress inputs/compress.in` will provide different results.
# ACTION NEEDED: If the path is different, please update it here.
PATH2LIB=~/effective-partial-redundancy-elimination/build/mypass/LLVMPJT.so        # Specify your build directory in the project
#
opt -enable-new-pm=0 -o ${1}_opt_dis_out_final.profdata.bc -load ${PATH2LIB} -PRE < ${1}_opt_dis_out_final.profdata.bc > /dev/null