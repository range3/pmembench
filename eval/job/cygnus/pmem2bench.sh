#!/bin/bash

function timestamp() {
  date +%Y.%m.%d-%H.%M.%S
}

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE:-$0}" )" &> /dev/null && pwd )"
SCRIPT_FILE_NAME="$( basename -- "$0" )"
PROJECT_DIR="$( cd "${SCRIPT_DIR}/../../.." &> /dev/null && pwd )"

OUTPUT_DIR="${PROJECT_DIR}/eval/raw/cygnus/pmem2bench/$(timestamp)"
mkdir -p "${OUTPUT_DIR}"
cd "${OUTPUT_DIR}"

JOB_FILE="${SCRIPT_DIR}/${SCRIPT_FILE_NAME%.*}.job.sh"

block_size_list=(
  64 128 256 512
  1K 2K 4K 8K 16K 32K 64K 128K 256K 512K
  1M
)

# nthreads_list=(
#   1 2 4 8 16 32 48
# )

param_source_list=(
  devdax
  # fsdax
  # anon
)

access_pattern_list=(
  seq
  random
)

queue_name="pmem-b"
devdax_opt="-v USE_DEVDAX=pmemkv -v NUM_DEVDAX=1"
# devdax_opt=""

for param_source in "${param_source_list[@]}"; do
# for nthreads in "${nthreads_list[@]}"; do
for block_size in "${block_size_list[@]}"; do
for ntstore in true false; do
for access_pattern in "${access_pattern_list[@]}"; do

  # -v PMEM2BENCH_NTHREADS=${nthreads} \
qsub \
  -q ${queue_name} \
  -A NBB \
  -b 1 \
  ${devdax_opt} \
  -v PROJECT_DIR="${PROJECT_DIR}" \
  -v OUTPUT_DIR="${OUTPUT_DIR}" \
  -v PMEM2BENCH_SOURCE=${param_source} \
  -v PMEM2BENCH_BLOCK_SIZE=${block_size} \
  -v PMEM2BENCH_NTSTORE=${ntstore} \
  -v PMEM2BENCH_ACCESS_PATTERN=${access_pattern} \
  ${JOB_FILE}

done
done
done
done
# done
