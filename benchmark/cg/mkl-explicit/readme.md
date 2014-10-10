Method
======
 Conjugate Gradient with diagonal preconditioner

Implementation
==============

 * Body: explicit code of CG; explicitly managing vectors and BLAS L1 ops
 * Matrix multiply: MKL implementation via MKL Sparse Blas

TODO
====

 * fix vector updates to BLAS L1 calls