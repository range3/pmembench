#!/bin/bash

for nthreads in 1 2 4 8 16; do
for block_size in 64 128 256 512 1K 2K 4K 8K 16K 32K 64K 128K 256K 512K 1M; do

qsub \
  -v OUTPUT_DIR \
  -v NTHREADS=${nthreads} \
  -v BLOCK_SIZE=${block_size} \
  ${JOB_SCRIPT}

done
done
