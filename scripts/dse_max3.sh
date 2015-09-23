#!/bin/env bash
function setBuild() {
  numPipes=$1
  inputWidth=$2
  cacheSize=$3
  maxRows=$4
  designName=spark-${numPipes}x${inputWidth}_${cacheSize}c_${maxRows}10kr
  echo "Setting build $designName"
  cp sparkdse ${designName} -r
  cd ${designName}
  javaCrap="private static final int"
  sed "s/${javaCrap} INPUT_WIDTH = .*;/${javaCrap} INPUT_WIDTH = $inputWidth;/g" -i src/spmv/src/SpmvManager.maxj
  sed "s/${javaCrap} NUM_PIPES = .*;/${javaCrap} NUM_PIPES= $numPipes;/g" -i src/spmv/src/SpmvManager.maxj
  sed "s/${javaCrap} cacheSize = .*;/${javaCrap} cacheSize = $cacheSize;/g" -i src/spmv/src/SpmvManager.maxj
  sed "s/${javaCrap} MAX_ROWS = .*;/${javaCrap} MAX_ROWS = ${maxRows}0000;/g" -i src/spmv/src/SpmvManager.maxj

  mkdir build
  cd build

  export CC=/opt/gcc-4.9.2/bin/gcc
  export CXX=/opt/gcc-4.9.2/bin/g++
  export LD_LIBRARY_PATH=/opt/gcc-4.9.2/lib64/:${LD_LIBRARY_PATH}
  source /vol/cc/opt/maxeler/maxcompiler-2013.2.2/settings.sh
  export MAXELEROSDIR=${MAXCOMPILERDIR}/lib/maxeleros-sim/lib

  cmake ..
  make spmvlib_dfe
  cd ../../
}

for i in \
  3,4,2048,7 \
  3,8,2048,7
do
  IFS=","; set $i
  setBuild $1 $2 $3 $4 &
done

