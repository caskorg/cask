#!/bin/sh

## ask PBS for time (format hh:mm:ss)
#PBS -l walltime=10:00:0
#PBS -l select=1:ncpus=1:mem=32gb

##load application modules if available
if hash module 2>/dev/null; then
  module load intel-suite/2015.3
  module load mpi/intel-5.0.2.044
fi

matrixBasePath=$HOME/matrices-eigen
#matrixBasePath=$HOME/test-matrices
if [ "$#" -eq 1 ]; then
  matrixList=$1
else
  matrixList=$(find . ${matrixBasePath} -name "*.mtx" -not -name "*_b.mtx" -exec basename {} \; | sed 's/_SPD.mtx//g')
fi

##command line
precList="ilu jacobi none"
factorSolverPackages="superlu mumps umfpack klu" # cholmod pastix superlu_dist

cd $HOME/workspaces/sparse-bench/src/cpu/petsc
source setup.sh
petscCommonFlags="-ksp_view -ksp_converged_reason -ksp_monitor_true_residual -ksp_initial_guess_nonzero -log_view"

for m in $matrixList; do
  matrix=${matrixBasePath}/${m}_SPD.mtx
  vector=${matrixBasePath}/${m}_b.mtx
  exp=${matrixBasePath}/${m}_x.mtx

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

  for p in $precList; do
    echo "Run parameters matrix='$m', precon='$p' solver='CG'"
    ./petsc_ksp -fin ${matrix} -vin ${vector} -ein ${exp} -ksp_type cg -pc_type ${p} ${petscCommonFlags}
  done

  # --- Direct solvers
  echo "Run parameters matrix='$m', precon='lu', solver='PETSC LU'"
  ./petsc_ksp -fin ${matrix} -vin ${vector} -ein ${exp} -ksp_type preonly -pc_type lu ${petscCommonFlags}

  for s in $factorSolverPackages; do
    echo "Run parameters matrix='$m', precon='lu', solver='$s'"
    ./petsc_ksp -fin ${matrix} -vin ${vector} -ein ${exp} -ksp_type preonly -pc_type lu -pc_factor_mat_solver_package $s ${petscCommonFlags}
  done

done
