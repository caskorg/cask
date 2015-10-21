#!/usr/bin/env bash


# Run DSE

FILE=../test_out.json
ARCH_CFG=architecture_config.out

# 1. Run DSE tool

# 2. Convert DSE output to a list of MAX_BUILDPARAMS
sed -e '/"architecture_params"/,/}/!d' ${FILE} | xargs -n9 \
 | sed -e 's/architecture_params: //g' -e 's/{//g' -e 's/},//g' \
 | sed -e 's/:/=/g' -e 's/ //g' | sed -e 's/,/ /g' > ${ARCH_CFG}

# 3. Start a build
while read p; do
  echo "Starting build $p"
  MAX_BUILDPARAMS="$p" make -C ../src/spmv/build build
done < ${ARCH_CFG}

echo $results
