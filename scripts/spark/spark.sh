#!/usr/bin/env bash


# Run DSE

DSE_FILE=dse_out.json
ARCH_CFG=architecture_config.out
BUILD_DIR=../../src/spmv/build
JSON_PARAM_FILE=`readlink -e ../../params.json`
BENCH_FILE=`readlink -e ../../test-benchmark`
DSE_CMD="./../../build/main ${BENCH_FILE} ${JSON_PARAM_FILE}"
DSE_LOG_FILE=dse.log

# 1. Run DSE tool
# TODO
${DSE_CMD} | tee ${DSE_LOG_FILE}

# 2. Convert DSE output to a list of MAX_BUILDPARAMS
sed -e '/"architecture_params"/,/}/!d' ${DSE_FILE} | xargs -n9 \
 | sed -e 's/architecture_params: //g' -e 's/{//g' -e 's/},//g' \
 | sed -e 's/:/=/g' -e 's/ //g' | sed -e 's/,/ /g' > ${ARCH_CFG}

# 3. Start a build
while read p; do
  echo "Starting build $p"
  MAX_BUILDPARAMS="$p" make -C ${BUILD_DIR} build
done < ${ARCH_CFG}

echo $results
