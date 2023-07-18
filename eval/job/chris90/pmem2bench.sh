#!/bin/bash
set -eux

function timestamp() {
  date +%Y.%m.%d-%H.%M.%S
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE:-$0}")" &>/dev/null && pwd)"
SCRIPT_FILE_NAME="$(basename -- "$0")"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../../.." &>/dev/null && pwd)"

OUTPUT_DIR="${PROJECT_DIR}/eval/raw/chris90/pmem2bench/$(timestamp)"
mkdir -p "${OUTPUT_DIR}"

cd "${PROJECT_DIR}"
spack env activate -d spack/envs/chris90

BENCHMARK_EXE="${PROJECT_DIR}/install/bin/pmem2bench"
size_per_thread=$((8 * 2 ** 30))

block_size_list=(
  64 128 256 512
  1K 2K 4K 8K 16K 32K 64K 128K 256K 512K
  1M
)

nthreads_list=(
  1 2 4 8 16
)

param_source_list=(
  devdax
  # fsdax
  # anon
)

access_pattern_list=(
  seq
  random
)

max_iter=3

for param_source in "${param_source_list[@]}"; do

  file_path="/dev/dax0.0"
  if [ "${param_source}" = "fsdax" ]; then
    file_path="/pmem/pmem2bench.bin"
  fi

  for block_size in "${block_size_list[@]}"; do
    for ntstore in true false; do

      ntstore_opt=""
      ntstore_label="t"
      if [ "${ntstore}" = true ]; then
        ntstore_opt="--non-temporal"
        ntstore_label="nt"
      fi

      if [ "${ntstore}" = true ]; then
        memcpy_opt="$ntstore_opt"
      else
        memcpy_opt="--noflush"
      fi

      for access_pattern in "${access_pattern_list[@]}"; do

        random_access_opt=""
        if [ "${access_pattern}" = "random" ]; then
          random_access_opt="--random"
        fi

        for nthreads in "${nthreads_list[@]}"; do
          total_size=$((nthreads * size_per_thread))

          for iter in $(seq $max_iter); do

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
              >"${OUTPUT_DIR}/w_${access_pattern}_${ntstore_label}_${param_source}_${block_size}_${nthreads}_${iter}.json"

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
              >"${OUTPUT_DIR}/r_${access_pattern}_${ntstore_label}_${param_source}_${block_size}_${nthreads}_${iter}.json"
          done

          if [ "${param_source}" = "fsdax" ]; then
            rm $file_path
          fi

        done

      done
    done
  done
done
