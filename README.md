sparse-bench
============

Benchmarking sparse linear algebra libraries on CPU, GPU and FPGA.

Real-life sparse matrices from this
[collection](http://www.cise.ufl.edu/research/sparse/matrices/) are
used.


Prerequisites
=============

You should have installed:

1. For CPU benchmarking:
- [armadillo](http://arma.sourceforge.net/) and its dependencies
- [PetSC](http://www.mcs.anl.gov/petsc/)
- [trilinos](http://trilinos.sandia.gov/)
- Note! For maximum performance these libraries usually require
vendor specific libraries (e.g. Intel MKL) or optimized linear
algebra packs (see their documentation for details)

2. For GPU benchmarking:
- [CUDA](http://www.nvidia.com/object/cuda_home_new.html) and its dependecies
- [cusp](https://github.com/cusplibrary/cusplibrary)
- Note! cuda external libraries (such as cusp) are assumed to be
installed in the user's home directory (`~/cuda`); you can specify
a different directory using the `CUDA_PATH` Makefile variable

3. For FPGA benchmarking:
- coming soon...

4. Other:
- python2.7 (including the wget package)


Installation
============

After installing dependencies run `

```
git clone https://github.com/paul-g/sparse-bench.git`
cd sparse-bench/benchmark/<specific benchmark>
make
./<binary name>
```

That's it!


Running the Benchmarks
======================

TODO: add a script to run all the benchmarks and produce some nice results.
Coming soon...
