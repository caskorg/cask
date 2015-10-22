#!/usr/bin/env bash


# Run DSE

DSE_FILE=../../test_out.json
ARCH_CFG=architecture_config.out
BUILD_DIR=../../src/spmv/build build

# 1. Run DSE tool
# TODO

# 2. Convert DSE output to a list of MAX_BUILDPARAMS
sed -e '/"architecture_params"/,/}/!d' ${DSE_FILE} | xargs -n9 \
 | sed -e 's/architecture_params: //g' -e 's/{//g' -e 's/},//g' \
 | sed -e 's/:/=/g' -e 's/ //g' | sed -e 's/,/ /g' > ${ARCH_CFG}

# 3. Start a build
while read p; do
  echo "Starting build $p"
  MAX_BUILDPARAMS="$p" make -C ${BUILD_DIR}
done < ${ARCH_CFG}

echo $results
