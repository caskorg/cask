# spark

`spark` is a tool for exploring customised architectures for sparse linear algebra. It focuses on micro-architectural optimsations for iterative solvers and the sparse matrix vector multiplication kernel.

## Installation

```
git clone https://github.com/paul-g/spark.git
git submodule update --init
cd spark && mkdir build && cd build && cmake ..
make -C main
```

## Requires

All builds are performed on the local machine which must have:

1. `Maxeler MaxCompiler 2013.2.2` or higher
2. `g++ 4.9.2` or higher
3. `Boost 1.55` or higher

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

__Note__ Some simulation tests may take a long time to run, particularly if a large architecture is simulated. A sample run is shown below, 
