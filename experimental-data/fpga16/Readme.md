## About

Experimental results for [presentation at FPGA 16](http://www.doc.ic.ac.uk/~pg1709/pg16fpga.pdf).

These results were obtained with revision 078afee of CASK.

## Benchmark

The benchmark can be downloaded from
[here](http://www.doc.ic.ac.uk/~pg1709/experimental-data/fpga16/fpga16.tar.gz).

Alternatively consider downloading the matrices directly from the [UoF Sparse
Matrix Collection](https://www.cise.ufl.edu/research/sparse/matrices/).

## Reproducing

To reproduce these results,

1. download the benchmark in `path/to/benchmark`
2. run with `cd scripts && python spark.py -t sim -p ../experimental-data/fpga16/params.json -b <path/to/benchmark> -d -rb`
