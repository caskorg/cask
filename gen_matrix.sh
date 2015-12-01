matrices=$(find test-benchmark -name '*.mtx')
outputDir=matrices

rm -f ${summaryFile}
rm -rf ${outputDir}
mkdir ${outputDir}

echo "Benchmarking..."

for f in ${matrices}
do
  matrix=$(basename ${f%.mtx})
  matrix_dir=${outputDir}/${matrix}
  echo ${matrix_dir}" "${matrix}
  mkdir ${matrix_dir}

  # Matrix summary
  matrix_summary=${matrix_dir}/summary.csv
  python sparsegrind/sparsegrind/main.py -f mm -a summary ${f} >> ${matrix_summary}

  # Sparsity plot
  python sparsegrind/sparsegrind/main.py -f mm -a plot ${f}
  sparsity_plot=${matrix_dir}/${matrix}.png
  cp sparsity.png ${sparsity_plot}

  # Simulation results
  matrix_sim_run=${matrix_dir}/sim_run.csv
  cd scripts && bash simrunner ../build/test_spmv_sim ../${f} >> ../${matrix_sim_run} && cd ..
done
