% Spark Design Document

# Introduction

Spark is a library for sparse linear algebra architectures which uses
customised implementations on FPGA (and eventually CPU and GPU).  Spark
simplifies and if necessary _abstracts_ the generation and tuning process for
sparse linear algebra problems based on the problem instance(s).

This document presents the Spark flow. The flow is exemplified on the Sparse
Matrix Vector Multiplication Kernel (SpMV kernel). The flow can similarly be
applied to any Kernel implemented in Spark (presently Conjugate Gradient and Bi
Conjugate Gradient implementations are _planned_ and considered feasible but
not implemented).

# Using Spark for problem specific tunning

Spark can be used to generate custom architectures based on given sparse
benchmark.


# Spark Usage

The example below shows how to use the SparkSpmv implementation from the client
perspective. Similarly it can be extended to support other calls such as
Conjugate Gradient directly etc.

``` {.cpp}
#include <Spark/Spmv>

void doSmth() {
  EigenSparseVec x;
  EigenSparseMatrix mat;

  // the implementation hint is used to narrow down the
  // search space; optionally the _force_ flag could be set
  // to force a certain implementation
  SparkImplHint hint(...);
  SparkSpmv spmv(hint); // could use arguments to select impl

  // analyze and pre-process step allow Spark to
  // select optimal implementation internally

  spmv.analyze(mat);
  spmv.preprocess(mat);
  spmv.load(mat);

  for (int i = 0; i < its; i++) {
    EigenSparseVec v = spmv.run(x);
    // do smth with v
  }
}
```

Two possibilities:

- provide C interface + library

- provide C++ interface + class + library
  - some elements still need to be provided as lib
  - for example the maxeler implementations


```
include/
  Spmv.cpp
    spmv(mat, v)
  BiCg.cpp

src/
 spmv/
   dfe/
     SpmvDfeInterface.cpp
   cpu/
   gpu/
   common/
     Preprocessing.hpp (?)
     Preprocessing.cpp
       Compression
       Blocking
       Partitionining
       Scheduling
       etc.
  cg/
    dfe/
      SpmvCgInterface.cpp
    cpu/
    common/
```
