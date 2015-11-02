#!/usr/bin/env bash

f=${1}
path="/mnt/data/scratch/pg1709/workspaces/spark/scripts"
machine="maxnode"

ssh ${machine} /bin/bash << EOF
 cd ${path}
 pwd
 export LD_LIBRARY_PATH=/opt/gcc-4.9.2/lib64:${LD_LIBRARY_PATH}
 ./hwrunner ../build/test_spmv_dfe ${f} | tee run.log
EOF