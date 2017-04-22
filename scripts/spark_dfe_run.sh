#!/usr/bin/env bash

f=${1}
path=`pwd`
# NB machine name depends on build type, currently this needs to be updated
# manually
machine="maia01"
# machine="maxnode2"

ssh ${machine} /bin/bash << EOF
 cd ${path}/..
 pwd
 export LD_LIBRARY_PATH=/opt/gcc-4.9.2/lib64:${LD_LIBRARY_PATH}
 src/frontend/hwrun_maia ${path}/test_spmv_dfe ${f} | tee run.log
 sleep 5
EOF
