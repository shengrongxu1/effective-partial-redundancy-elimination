#!/bin/bash
### run.sh
### Benchmark Runner Script
### Locate this script at each benchmark directory. e.g, 583simple/run.sh
### Usage: sh run.sh ${benchmark_name} ${input} 
### Usage: sh run.sh compress compress.in OR sh run.sh simple OR sh run.sh wc cccp.c
### Note: Do NOT include inputs/ in ${input}, `./run.sh compress inputs/compress.in` will provide different results.

PATH_MYPASS=~/HW1/build/hw1pass/LLVMHW1.so ### Action Required: Specify the path to your pass ###
NAME_MYPASS=-hw1 ### Action Required: Specify the name for your pass ###
BENCH=src/${1}.c
INPUT=${2}

setup(){
if [ ! -z "${INPUT}" ]; then
echo "INPUT:${INPUT}"
ln -sf input1/${INPUT} .
fi
}

cleanup(){
rm *.profdata
rm *.bc
rm *.prof
rm *.profraw
if [ ! -z "${INPUT}" ]; then
rm *.in
rm *.in.Z
fi
}

# Clean up files from previous runs
cleanup
# Prepare input to run
setup

# Convert source code to bitcode (IR) using clang
# This approach has an issue with -O2, so we are going to stick with default optimization level (-O0)
clang -emit-llvm -c ${BENCH} -o ${1}.bc -Wno-everything

# Generate binary executable with profiler embedded
clang -fprofile-instr-generate ${BENCH} -o ${1}.prof -Wno-everything

# Collect profiling data
# Running the executable generated in the previous step will create the profile data in default.profraw.
./${1}.prof ${INPUT} > /dev/null

# Translate raw profiling data into LLVM data format
llvm-profdata merge -output=${1}.profdata default.profraw

# Use the profiling data to create a new executable with profile info.
clang -emit-llvm -fprofile-instr-use=${1}.profdata ${BENCH} -c -o ${1}.prof.bc -Wno-everything

# Apply your pass to bitcode (IR)
opt -enable-new-pm=0 -pgo-test-profile-file=${1}.profdata -load ${PATH_MYPASS} ${NAME_MYPASS} < ${1}.prof.bc > /dev/null