#!/usr/bin/env bash

# Script to drive the DSE process
# TODO compilation phase requires to update the Device header
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

# TODO choose target based on command param
target="DFE_SIM"

# 3. Start a build
while read p; do
  echo "Starting build $p"
  buildLocation=`MAX_BUILDPARAMS="${p} target=${target}" make -C ${BUILD_DIR} maxfile | grep 'Build location'`

  buildDir=`echo ${buildLocation} | grep -o '/.*'`
  echo "Build dir = ${buildDir}"

  maxFileLocation=`grep "MaxFile:" ${buildDir}/_build.log | grep -o '/.*max'`
  echo "MaxFile Location = ${maxFileLocation}"

  # compile the maxfile
	sliccompile ${maxFileLocation} "maxfile.o"

  # create the device library
  PRJ=Spmv
  LIB=lib${PRJ}_sim.so
  MAX_INC="-I${MAXCOMPILERDIR}/include -I${MAXCOMPILERDIR}/include/slic -I${MAXELEROSDIR}/include"
  GEN_INC="-I${buildDir}/results/"
  CFLAGS="-Wall ${MAX_INC} -I${DFESNIPPETS}/include -I../../include -std=c++11 ${GEN_INC}"
	g++ -I${SIMMAXDIR} ${CFLAGS} -c ../../src/spmv/src/SpmvDeviceInterface.cpp -o SpmvDeviceInterface.o
  LFLAGS="-L${MAXCOMPILERDIR}/lib -L${MAXELEROSDIR}/lib -lmaxeleros -lslic -lm -lpthread"
  g++ -fPIC --std=c++11 -shared -Wl,-soname,${LIB}.0 -o ${LIB} "maxfile.o" SpmvDeviceInterface.o ${LFLAGS}

  # copy the generated library
  cp ${LIB} ../../lib-generated
  cp ${LIB} ../../lib-generated/${LIB}.0

  # build the "client" code
  make -C ../../build/ test_spmv_sim

  # run the benchmark
  cd ../../build/
  # TODO run all benchmarks
  MAX_BUILDPARAMS="${p}" ../scripts/simrunner ./test_spmv_sim ../test-matrices/test_dense_48.mtx
  cd ../scripts/spark

done < ${SIM_CFG}

echo $results
