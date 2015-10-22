#!/usr/bin/env bash

# Script to drive the DSE process
# TODO compilation phase requires to update the Device header
# TODO generate build name for simulation as for hardware
# TODO provide flag to run hardware build / perf tests
# TODO force compilation for builds with small timing failures

# Run DSE

DSE_FILE=dse_out.json
ARCH_CFG=architecture_config.out
BUILD_DIR=../../src/spmv/build
JSON_PARAM_FILE=`readlink -e ../../params.json`
BENCH_FILE=`readlink -e ../../test-benchmark-tiny`
DSE_CMD="./../../build/main ${BENCH_FILE} ${JSON_PARAM_FILE}"
DSE_LOG_FILE=dse.log

# 1. Run DSE tool
# TODO
${DSE_CMD} | tee ${DSE_LOG_FILE}

# 2. Convert DSE output to a list of MAX_BUILDPARAMS
sed -e '/"architecture_params"/,/}/!d' ${DSE_FILE} | xargs -n9 \
 | sed -e 's/architecture_params: //g' -e 's/{//g' -e 's/},//g' \
 | sed -e 's/:/=/g' -e 's/ //g' | sed -e 's/,/ /g' > ${ARCH_CFG}

SIM_CFG=sim_architecture_config.out
head -n 1 ${ARCH_CFG} > ${SIM_CFG}

# 3. Start a build
  set -x
while read p; do
  echo "Starting build $p"
  buildLocation=`MAX_BUILDPARAMS="${p} target=DFE_SIM" make -C ${BUILD_DIR} maxfile | grep 'Build location'`
  set +x

  buildDir=`echo ${buildLocation} | grep -o '/.*'`
  echo "Build dir = ${buildDir}"

  maxFileLocation=`grep "MaxFile:" ${buildDir}/_build.log | grep -o '/.*max'`
  echo "MaxFile Location = ${maxFileLocation}"

  # generate Spmv implementation
  PRJ=Spmv # TODO need a name?
  LIB=lib${PRJ}_sim.so
	sliccompile ${maxFileLocation} "maxfile.o"
  LFLAGS="-L${MAXCOMPILERDIR}/lib -L${MAXELEROSDIR}/lib -lmaxeleros -lslic -lm -lpthread"
  g++ -fPIC --std=c++11 -shared -Wl,-soname,${LIB}.0 -o ${LIB} "maxfile.o" ${LFLAGS}
  cp ${LIB} ../../lib-generated
  cp ${LIB} ../../lib-generated/${LIB}.0
  make -C ../../build/ test_spmv_sim

  # run the benchmark
  cd ../../build/
  # TODO run all benchmarks
  ../scripts/simrunner ./test_spmv_sim ../test-matrices/test_dense_48.mtx
  cd ../scripts/spark

done < ${SIM_CFG}

echo $results
