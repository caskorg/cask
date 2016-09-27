#!/bin/sh

## ask PBS for time (format hh:mm:ss)
#PBS -l walltime=02:00:0
#PBS -l select=1:ncpus=1:mem=16gb

##load application modules if available
if hash module 2>/dev/null; then
  module load intel-suite/2015.3
  module load mpi/intel-5.0.2.044
fi

matrixBasePath=$HOME/matrices-eigen
if [ "$#" -eq 1 ]; then
  matrixList=$1
else
  matrixList=$(find . ${matrixBasePath} -name "*.mtx" -not -name "*_b.mtx" -exec basename {} \; | sed 's/_SPD.mtx//g' | head -n 1)
fi


##command line
precList="ilu jacobi none"
factorSolverPackages="pastix superlu_dist superlu mumps umfpack klu cholmod"

source setup.sh
cd $HOME/workspaces/sparse-bench/src/cpu/petsc

for m in $matrixList; do
  matrixName=$m
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

  for p in $precList; do
    echo "Run parameters matrix='$m', precon='$p' solver='CG'"
    bash run.sh $m $p
  done
  ./petsc_ksp -fin ${matrix} -vin ${vector} -ksp_type cg -ksp_view -ksp_converged_reason -ksp_monitor_true_residual -ksp_initial_guess_nonzero -log_view -pc_type ${pcType}


  # --- Direct solvers
  echo "Run parameters matrix='$m', precon='lu', solver='PETSC LU'"
  ./petsc_ksp -fin ${matrix} -vin ${vector} -ksp_type preonly -ksp_view -ksp_converged_reason -ksp_monitor_true_residual -ksp_initial_guess_nonzero -log_view -pc_type lu

  for s in $factorSolverPackages; do
    echo "Run parameters matrix='$m', precon='lu', solver='$s'"
    ./petsc_ksp -fin ${matrix} -vin ${vector} -ksp_type preonly -ksp_view -ksp_converged_reason -ksp_monitor_true_residual -ksp_initial_guess_nonzero -log_view -pc_type lu -pc_factor_mat_solver_package $s
  done

done
