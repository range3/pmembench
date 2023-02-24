#!/bin/bash
set -eux

function timestamp() {
  date +%Y.%m.%d-%H.%M.%S
}

function addtimestamp() {
    while IFS= read -r line; do
        printf '%s %s\n' "$(timestamp)" "$line";
    done
}

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE:-$0}" )" &> /dev/null && pwd )"
PROJECT_DIR="$( cd "${SCRIPT_DIR}/../../.." &> /dev/null && pwd )"
OUTPUT_DIR="${PROJECT_DIR}/eval/raw/pegasus/pmem2bench/$(timestamp)"
mkdir -p "${OUTPUT_DIR}"
# SCRIPT_FILE_NAME="$( basename -- "$0" )"
# JOB_FILE="${SCRIPT_DIR}/${SCRIPT_FILE_NAME%.*}.job.sh"

cd "${PROJECT_DIR}"
spack env activate -d spack/envs/pegasus

BENCHMARK_EXE="${PROJECT_DIR}/install/bin/pmem2bench"

block_list=(
  256 512
  1K 2K 4K 8K 16K 32K 64K 128K 256K 512K
  1M 2M 4M 8M 16M
)

nthreads_list=(
  1 2 4 8 16 32 48
  # 16
)

size_per_thread=$((8*2**30))

for nthreads in "${nthreads_list[@]}"; do
for block in "${block_list[@]}"; do
  total_size=$((nthreads * size_per_thread))
  "${BENCHMARK_EXE}" \
    --source devdax \
    --path /dev/dax0.0 \
    --nthreads $nthreads \
    --file_size $total_size \
    --total $total_size \
    --stripe $total_size \
    --block $block \
    --non-temporal \
    --write \
    --prettify \
      > "${OUTPUT_DIR}/w_${block}_${nthreads}.json"
  "${BENCHMARK_EXE}" \
    --source devdax \
    --path /dev/dax0.0 \
    --nthreads $nthreads \
    --file_size $total_size \
    --total $total_size \
    --stripe $total_size \
    --block $block \
    --non-temporal \
    --read \
    --prettify \
      > "${OUTPUT_DIR}/r_${block}_${nthreads}.json"
done
done
