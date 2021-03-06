#!/usr/bin/env bash

# Script to drive the DSE process

function usage {
  echo "Usage: $0 dfe|sim bench-path dse-params"
  echo ""
  echo "  where:"
  echo "    dfe|sim     -- whether to build simulation or hardware"
  echo "    bench-path  -- path to benchmark directory"
  echo "    dse-params  -- path to a JSON dse param file"
  echo ""
  echo "  example:"
  echo "    $0 sim ../../test-benchmark ../../dse_params.json"
}

DSE_FILE=dse_out.json
BUILD_DIR=../src/spmv/build

PARAM_TARGET=$1
if [ "${PARAM_TARGET}" == "dfe" ]
then
  target="DFE"
elif [ "${PARAM_TARGET}" == "sim" ]
then
  target="DFE_SIM"
else
  echo "Error: Target must be either 'dfe' or 'sim'"
  exit 1
fi

BENCH_FILE=`readlink -e ${2}`
if [ "$?" != "0" ]
then
  echo "Error: Benchmark directory not found!"
  exit 1
fi

JSON_PARAM_FILE=`readlink -e ${3}`
if [ "$?" != "0" ]
then
  echo "Error: JSON param file not found!"
  exit 1
fi

ARCH_CFG=${PARAM_TARGET}_architecture_config.out
DSE_CMD="./../build/main ${BENCH_FILE} ${JSON_PARAM_FILE}"
DSE_LOG_FILE=dse.log

# 1. Run DSE tool
${DSE_CMD} | tee ${DSE_LOG_FILE}

# 2. Convert DSE output to a list of MAX_BUILDPARAMS
sed -e '/"architecture_params"/,/}/!d' ${DSE_FILE} | xargs -n11 \
 | sed -e 's/architecture_params: //g' -e 's/{//g' -e 's/},//g' \
 | sed -e 's/:/=/g' -e 's/ //g' | sed -e 's/,/ /g' > ${ARCH_CFG}

# 3. Start a build
while read p; do
  PRJ="Spmv"
  echo "Starting build $p"
  buildName="${PRJ}_${PARAM_TARGET}_"`echo "${p}" | sed 's/_//g' | sed 's/ /_/g' | sed 's/=//g'`
  buildDir="${BUILD_DIR}/${buildName}"
  maxFileLocation="${buildDir}/results/${PRJ}.max"
  maxFileTarget="${buildName}/results/${PRJ}.max"
  echo "MaxFile Location = ${maxFileLocation}"

  # Force make to always rebuilds, because the design parameters will change
  # but are not being picked up by make. Set this to "" to avoid needless
  # recompilation during development & testing
  #alwaysBuild="-B"
  buildParams="${p} target=${target} buildName=${buildName} maxFileName=${PRJ}"
  MAX_BUILDPARAMS="${buildParams}" make ${alwaysBuild} -C ${BUILD_DIR} ${maxFileTarget}

  # compile the maxfile
	sliccompile ${maxFileLocation} "maxfile.o"

  # create the device library
  LIB=lib${PRJ}_${PARAM_TARGET}.so

  # Ignore timinig score
  echo "Old timing score"
  grep TIMING_SCORE ${maxFileLocation}
  sed  -i -e s/PARAM\(TIMING_SCORE\,.*\)/PARAM\(TIMING_SCORE,\ 0\)/ ${maxFileLocation}
  echo "New timing score"
  grep TIMING_SCORE ${maxFileLocation}

  MAX_INC="-I${MAXCOMPILERDIR}/include -I${MAXCOMPILERDIR}/include/slic -I${MAXELEROSDIR}/include"
  GEN_INC="-I${buildDir}/results/"
  CFLAGS="-Wall ${MAX_INC} -I${DFESNIPPETS}/include -I../include -std=c++11 ${GEN_INC}"
	g++ -I${SIMMAXDIR} ${CFLAGS} -c ../src/spmv/src/SpmvDeviceInterface.cpp -o SpmvDeviceInterface.o
  LFLAGS="-L${MAXCOMPILERDIR}/lib -L${MAXELEROSDIR}/lib -lmaxeleros -lslic -lm -lpthread"
  g++ -fPIC --std=c++11 -shared -Wl,-soname,${LIB}.0 -o ${LIB} "maxfile.o" SpmvDeviceInterface.o ${LFLAGS}

  # copy the generated library
  cp ${LIB} ../lib-generated
  cp ${LIB} ../lib-generated/${LIB}.0

  # build the "client" code
  make -C ../build/ test_spmv_${PARAM_TARGET}

  # run the benchmark
  # TODO run all benchmarks
  find ${BENCH_FILE} -name "*.mtx" > benchmark_matrices.out
  resFile=`echo "${p}" | sed -e 's/ /_/g'`"_${PARAM_TARGET}_total.out"
  rm -f ${resFile} # cleanup previous results
  while read f; do

     if [ "${PARAM_TARGET}" == "dfe" ]
     then
       # Run remotely, assumes ssh keys set up
       # TODO support max4
       path="/mnt/data/scratch/pg1709/workspaces/spark/scripts"
       ssh maxnode2 /bin/bash << EOF
         cd /mnt/data/scratch/pg1709/workspaces/spark/scripts
         pwd
         export LD_LIBRARY_PATH=/opt/gcc-4.9.2/lib64:${LD_LIBRARY_PATH}
         ./hwrunner ../build/test_spmv_${PARAM_TARGET} ${f} | tee run.log
EOF
     else
       # Run locally in simulation
       ./simrunner ../build/test_spmv_${PARAM_TARGET} ${f} | tee run.log
     fi

     # TODO log per partition results

     # log total results
     value=`grep Iterations run.log`
     fileBase=`basename $f`
     echo "Results file > ${resFile}"
     echo "${fileBase},${value}" >> "${resFile}"
  done < benchmark_matrices.out

done < ${ARCH_CFG}
