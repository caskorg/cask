#!/usr/bin/env bash

f=${1}
path=`pwd`
# XXX machine name depends on build type
machine="maia01"
# machine="maxnode2"

echo "Path is ${path}"

ssh ${machine} /bin/bash << EOF
 cd ${path}/..
 pwd
 export LD_LIBRARY_PATH=/opt/gcc-4.9.2/lib64:${LD_LIBRARY_PATH}
 src/frontend/hwrun_maia ${path}/test_spmv_dfe ${f} | tee run.log
 sleep 5
EOF
