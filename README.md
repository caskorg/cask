# CASK - Custom Architectures for Sparse Kernels

[![Build Status](https://travis-ci.org/caskorg/cask.svg?branch=master)](https://travis-ci.org/caskorg/cask/)

`CASK` is a tool for exploring customised architectures for sparse algebra. It
focuses on micro-architectural optimsations for iterative solvers and the
sparse matrix vector multiplication kernel.

`CASK` is released under the MIT license - use it as you please, but we assume
no responsibility. Also, if you find the work interesting and this project
useful, we kindly ask that you cite the following:

_Paul Grigoras, Pavel Burovskiy, Wayne Luk_, [CASK: Open-Source Custom
Architectures for Sparse Kernels](http://dl.acm.org/citation.cfm?doid=2847263.2847338). FPGA 2016.

```
@inproceedings{grigoras2015cask,
  author = {Grigoras, Paul and Burovskiy, Pavel and Luk, Wayne},
  title = {{CASK}: Open-Source Custom Architectures for Sparse Kernels},
  booktitle = FPGA,
  year = {2016},
  pages = {179--184},
}
```

The main steps in CASK are:

1. Design Space Exploration (DSE) - given a benchmark and a set of
   architectures find the optimal instantiations for each architecture
2. Build - generate an implementation library from the architectures which can
   be used in an application
3. Benchmark - measure performance of the architectures
4. Collect and post-process results - collects results of various steps in the
   process and displays them in a meaningful, user-friendly way

CASK supports three main flows available from the top-level makefile:

1. `mock-flow` - generates CPU based stubs for the architectures; as such it
   has minimal external dependencies and can run on a local machine without any
   FPGA vendor tools available; useful for developing the infrastructure as it
   skips the most expensive steps (building simulation & hardware
   implementations)
2. `sim-flow` - build simulation versions of the hardware implementations;
   these are useful for checking correctness as they will pick up most
   functional issues
4. `hw-flow` - build actual hardware (FPGA) implementations for the designs

Each of these flows takes as input:
- a sparse matrix benchmark
- a range of design parameters to explore

It produces as output:
- a shared library with the implementation
- benchmarking results

## Requirements

All flows run on the local machine and require:

1. `g++ 4.9.2` or higher (for C++11 support)
2. `Boost 1.55` or higher

To support `sim-flow` the local machine must also have:

1. `Maxeler MaxCompiler 2013.2.2` or higher

To support `hw-flow` the local machine must also have:

1. An installed FPGA card (Maxeler Dataflow Engine such as Vectis or Maia)
2. MaxelerOS version matching the MaxCompiler version used for the hardware
   build


## Installation

Install python deps:

```
virtualenv venv/ && source venv/bin/activate 
pip install -f requirements.txt
```

Clone this repository then run:

```
git submodule update --init
cd cask && mkdir build && cd build && cmake ..
make -C main
```


## Usage

### DSE flow

First, compile the DSE program:

````
mkdir build && cd build && cmake .. && make main
````

You can then use the DSE flow through `scripts/spark.py`.

```
bash-4.1$ python spark.py  -h
usage: spark.py [-h] [-d] -t {dfe,sim} -p PARAM_FILE -b BENCHMARK_DIR
                [-m MAX_BUILDS]

Run Spark DSE flow

optional arguments:
  -h, --help            show this help message and exit
  -d, --dse
  -t {dfe,sim}, --target {dfe,sim}
  -p PARAM_FILE, --param-file PARAM_FILE
  -b BENCHMARK_DIR, --benchmark-dir BENCHMARK_DIR
  -m MAX_BUILDS, --max-builds MAX_BUILDS
```

`spark.py` automates:

1. DSE process based on a set of benchmark matrices
2. generating simulation and hardware configurations
3. running and benchmarking simulation and hardware configurations


### CMake Flow

Once a simulation / hardware library has been generated through the DSE flow, it will be available in `lib-generated`.

From here you can use also CMake directly to rebuild the `client` targets, for example `test_spmv_sim`. Just build the corresponding Makefile target:

```
make -C build test_spmv_sim
```

### Running tests

Tests are provided for both software, hardware simulation and hardware runs. These should always pass on the `master` branch. Beware though that it's not practical to test every design parameter configuration, so awkward issues may arise.

First, compile the test binaries (e.g.): `make -C build test_spmv_sim`
Then you can:

1. Run unit tests with `ctest -R unit`
2. Run hardware simulation tests with `ctest -R sim`
3. Run hardware tests with `ctest -R hw`

__Note__ Some simulation tests may take a long time to run (`~60s), particularly if a large architecture is  simulated.
