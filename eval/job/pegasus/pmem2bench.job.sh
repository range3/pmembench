#!/bin/bash
#PBS -A NBB
#PBS -q gpu
#PBS -l elapstim_req=1:00:00
#PBS -T distrib 

set -eux

function timestamp() {
  date +%Y.%m.%d-%H.%M.%S
}

function addtimestamp() {
    while IFS= read -r line; do
        printf '%s %s\n' "$(timestamp)" "$line";
    done
}

param_source=$PMEM2BENCH_SOURCE
# nthreads=$PMEM2BENCH_NTHREADS
block_size=$PMEM2BENCH_BLOCK_SIZE
ntstore=$PMEM2BENCH_NTSTORE
access_pattern=$PMEM2BENCH_ACCESS_PATTERN

mkdir -p "${OUTPUT_DIR}"
# SCRIPT_FILE_NAME="$( basename -- "$0" )"
# JOB_FILE="${SCRIPT_DIR}/${SCRIPT_FILE_NAME%.*}.job.sh"

cd "${PROJECT_DIR}"
spack env activate -d spack/envs/pegasus

BENCHMARK_EXE="${PROJECT_DIR}/install/bin/pmem2bench"

size_per_thread=$((8*2**30))

ntstore_opt=""
ntstore_label="t"
if [ "${ntstore}" = true ]; then
  ntstore_opt="--non-temporal"
  ntstore_label="nt"
fi
random_access_opt=""
if [ "${access_pattern}" = "random" ]; then
  random_access_opt="--random"
fi

file_path="/dev/dax0.0"
if [ "${param_source}" = "fsdax" ]; then
  file_path="/pmem/pmem2bench.bin"
fi


nthreads_list=(
  1 2 4 8 16 32 48
)

max_iter=3

for nthreads in "${nthreads_list[@]}"; do
total_size=$((nthreads * size_per_thread))
for iter in $(seq $max_iter); do
  if [ "${ntstore}" = true ]; then
    memcpy_opt="$ntstore_opt"
  else
    # because Pegasus has FastADR (eADR), flush is no longer necessary.
    memcpy_opt="--noflush"
  fi

    # $memcpy_opt \
  "${BENCHMARK_EXE}" \
    --source $param_source \
    --path $file_path \
    --nthreads $nthreads \
    --stripe $total_size \
    --block $block_size \
    $ntstore_opt \
    $random_access_opt \
    --write \
    --prettify \
      > "${OUTPUT_DIR}/w_${access_pattern}_${ntstore_label}_${param_source}_${block_size}_${nthreads}_${iter}.json"

  "${BENCHMARK_EXE}" \
    --source $param_source \
    --path $file_path \
    --nthreads $nthreads \
    --stripe $total_size \
    --block $block_size \
    $memcpy_opt \
    $random_access_opt \
    --read \
    --prettify \
      > "${OUTPUT_DIR}/r_${access_pattern}_${ntstore_label}_${param_source}_${block_size}_${nthreads}_${iter}.json"
done

if [ "${param_source}" = "fsdax" ]; then
  rm $file_path
fi

done
