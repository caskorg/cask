sparse-bench
============

Benchmarking sparse linear algebra libraries on CPU, GPU and FPGA.

Real-life sparse matrices from this
[collection](http://www.cise.ufl.edu/research/sparse/matrices/) are
used.

`benchmark/<algorithm>` contains various implementations for the
algorithm (hand-written, Intel MKL, CUDA, etc.)

## Requires

<!-- Prerequisites -->
<!-- ============= -->

<!-- You should have installed: -->

<!-- 1. For CPU benchmarking: -->
<!--   - [armadillo](http://arma.sourceforge.net/) and its dependencies -->
<!--   - [PetSC](http://www.mcs.anl.gov/petsc/) -->
<!--   - [trilinos](http://trilinos.sandia.gov/) -->
<!--   - Note! For maximum performance these libraries usually require -->
<!--   vendor specific libraries (e.g. Intel MKL) or optimized linear -->
<!--   algebra packs (see their documentation for details) -->

* For CPU implementations
  * Intel MKL
* For GPU implementations
  * [CUDA](http://www.nvidia.com/object/cuda_home_new.html) and its dependecies
  * [cusp](https://github.com/cusplibrary/cusplibrary)

__Note__ Make sure you use a version of cusp that is compatible with
  CUDA (e.g. cusp 0.4.0 with CUDA 5.5)

__Note__ cuda external libraries (such as cusp) are assumed to be
  installed in the user's home directory (`~/cuda`); you can specify a
  different directory using the `CUDA_PATH` Makefile variable

<!-- 3. For FPGA benchmarking: -->
<!--   - coming soon... -->

<!-- 4. Other: -->
<!--   - python2.7 (including the wget package) -->


## Running

### CPU Implementations

You can build the benchmarks with:

```
mkdir build && cd build && cmake ..
```

The excutables will be stored in `build/bin`.

### FPGA Implementations

FPGA designs are compiled through separate targets (since they take significantly longer).
Use `make help` to list available targets and choose the one you need.

For example, to compile the FPGA design (generate a bitstream, C/C++ bindings and wrap them in a `.so`) you could do:

```
make fpga_naive_simbuild
```

You can run designs built for hardware directly.

To run simulation designs use the provided `run-sim.sh` which starts the simulator daemon, runs the design and stops the simulator:

```
bash run-sim.sin build/bin/fpgaNaive_sim
```
