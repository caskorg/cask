#ifndef STREAMGRID_DFE
#define STREAMGRID_DFE

#include <cstdio>
#include <vector>
#include <iostream>

// #include "Maxfiles.h"
#include "MaxSLiCInterface.h"

#ifdef __cplusplus
extern "C" {
#endif
// Forward declaration for dfe function call.
/**
 * \brief Basic static function for the interface 'default'.
 * \param [in] ticks_StreamGridKernel The number of ticks for which kernel "StreamGridKernel" will run.
 * \param [in] inscalar_StreamGridKernel_dt Input scalar parameter "StreamGridKernel.dt".
 * \param [in] instream_ex1E1 Stream "ex1E1".
 * \param [in] instream_size_ex1E1 The size of the stream instream_ex1E1 in bytes.
 * \param [in] instream_ex1KR Stream "ex1KR".
 * \param [in] instream_size_ex1KR The size of the stream instream_ex1KR in bytes.
 * \param [in] instream_ex1TR Stream "ex1TR".
 * \param [in] instream_size_ex1TR The size of the stream instream_ex1TR in bytes.
 * \param [in] instream_ex1U1 Stream "ex1U1".
 * \param [in] instream_size_ex1U1 The size of the stream instream_ex1U1 in bytes.
 * \param [out] outstream_res Stream "res".
 * \param [in] outstream_size_res The size of the stream outstream_res in bytes.
 */
void StreamGrid(
    uint64_t ticks_StreamGridKernel,
    double inscalar_StreamGridKernel_dt,
    const void *instream_ex1E1,
    size_t instream_size_ex1E1,
    const void *instream_ex1KR,
    size_t instream_size_ex1KR,
    const void *instream_ex1TR,
    size_t instream_size_ex1TR,
    const void *instream_ex1U1,
    size_t instream_size_ex1U1,
    void *outstream_res,
    size_t outstream_size_res);

#ifdef __cplusplus
}
#endif


std::vector<double> operator/(std::vector<double> a, std::vector<double> b) {
  std::vector<double> tmp(a.size());
  for (int i = 0; i < a.size(); i++)
    tmp[i] = a[i] / b[i];
  return tmp;
}

std::vector<double> operator+(std::vector<double> a, std::vector<double> b) {
  std::vector<double> tmp(a.size());
  for (int i = 0; i < a.size(); i++)
    tmp[i] = a[i] + b[i];
  return tmp;
}

std::vector<double> operator*(std::vector<double> a, double b) {
  std::vector<double> tmp(a.size());
  for (int i = 0; i < a.size(); i++)
    tmp[i] = a[i] * b;
  return tmp;
}

std::vector<double> operator*(std::vector<double> a, std::vector<double> b) {
  std::vector<double> tmp(a.size());
  for (int i = 0; i < a.size(); i++)
    tmp[i] = a[i] * b[i];
  return tmp;
}

std::vector<double> runCPU_oneStepEstimateEx1(
    std::vector<double> ex1U1,
    std::vector<double> ex1E1,
    std::vector<double> ex1KR,
    std::vector<double> ex1TR,
    double dt
    ) {
  return (ex1U1 * -1 + ex1KR * ex1E1) * dt / ex1TR + ex1U1;
}

void pad(std::vector<double> &v, long len) {
  while (v.size() < len)
    v.push_back(0);
}

long clm(long length, long bytes) {
  long r = length / bytes;
  if (length % bytes == 0)
   return length;
  return (r + 1) * bytes;
}

std::vector<double> runDFE_oneStepEstimateEx1(
    std::vector<double> ex1U1,
    std::vector<double> ex1E1,
    std::vector<double> ex1KR,
    std::vector<double> ex1TR,
    double dt
    ) {
  long initSize = ex1U1.size();
  long elems = clm(ex1U1.size(), 2);
  pad(ex1U1, elems);
  long len = ex1U1.size() * sizeof(double);
  std::vector<double> res(elems);
  StreamGrid(
      elems,
      dt,
      &ex1U1[0],
      len,
      &ex1E1[0],
      len,
      &ex1KR[0],
      len,
      &ex1TR[0],
      len,
      &res[0],
      len);
  res.resize(initSize);
  return res;
}

#endif /* end of include guard: STREAMGRID_DFE */

