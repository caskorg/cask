#!/usr/bin/env bash
source setup.sh

matrixName=$1
matrixBasePath=/home/paul-g/workspaces/sparsegrind/sparsegrind/matrices-eigen
matrix=${matrixBasePath}/${matrixName}_SPD.mtx
vector=${matrixBasePath}/${matrixName}_b.mtx

if [ ! -f $matrix ]; then
  echo "Error! Could not find file $matrix"
  echo "Usage ./$0 <matrix_base_name>"
  exit 1
fi

if [ ! -f $vector ]; then
  echo "Error! Could not find file $vector"
  echo "Usage ./$0 <matrix_base_name>"
  exit 1
fi

./petsc_ksp -fin ${matrix} -vin ${vector} -ksp_type cg -ksp_view -ksp_converged_reason -ksp_monitor_true_residual -ksp_initial_guess_nonzero
