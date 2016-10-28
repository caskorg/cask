Revision: `a153db3`

 ```
 $ matrix=olafu_SPD && MKL_NUM_THREADS=4 OMP_NUM_THREADS=1 time build/spam -mat /home/paul-g/workspaces/sparse-bench/test/all-systems/${matrix}.mtx -rhs  /home/paul-g/workspaces/sparse-bench/test/all-systems/${matrix}_b.mtx -lhs /home/paul-g/workspaces/sparse-bench/test/solutions/${matrix}_x.mtx
 Running without preconditioning 
 "setup took":"0.00229831",\n"iterations":"1999",\n"solve took":"1.4436",\n"estimated error":"0",\n"error":"7.92727",\n"bench repetitions":"0"\nRunning with ILU preconditioning 
 "setup took":"175.56",\n"iterations":"0",\n"solve took":"0.0135482",\n"estimated error":"0",\n"error":"7.95806",\n"bench repetitions":"0"\n178.90user 0.09system 2:57.67elapsed 100%CPU (0avgtext+0avgdata 94172maxresident)k
 29608inputs+600outputs (13major+23113minor)pagefaults 0swaps
 ```

