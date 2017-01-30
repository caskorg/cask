#!/usr/bin/env bash
resultFile=$1
if [ "$#" -ne 1 ]; then
  resultFile=`ls -t run.sh.o* -1 | head -n 1`;
  echo "Using most recent output file: ${resultFile}"
fi

tmpMat=petsc_run_err_mat_name.txt
tmpNorm=petsc_run_err_norm.txt
tmpTime=petsc_run_out_time.txt
result=result.out

grep 'nnz =' ${resultFile} -B 1 | grep -v 'nnz' | grep -v '-' | sed 's/ solver/, solver/g' | sed 's/Run parameters matrix=//g' | sed 's/precon=//g' | sed 's/solver=//g' > ${tmpMat}
# grep 'nnz =' ${resultFile} -B 1 | grep -v 'nnz' | grep -v '-' > ${tmpMat}
grep 'Norm of error' ${resultFile} | sed 's/Norm of error //g' | sed 's/ Iterations //g' > ${tmpNorm}
grep 'Time (sec):' ${resultFile}  | sed 's/Time (sec):           //g' | sed 's/      /,/g' | sed 's/   /,/g' > ${tmpTime}
paste -d"," ${tmpMat} ${tmpNorm} ${tmpTime} > ${result}

# sanitize the output a bit
sed -i 's/,,/,/g' ${result}

rm -f ${tmpMat} ${tmpNorm} ${tmpTime}
