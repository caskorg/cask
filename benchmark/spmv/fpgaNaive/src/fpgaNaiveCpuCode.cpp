/***
    This is a simple demo project that you can copy to get started.
    Comments blocks starting with '***' and subsequent non-empty lines
    are automatically added to this project's wiki page.
*/

#include <stdio.h>

#include <vector>
#include <iostream>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"

using namespace std;

int main(void)
{

  const int inSize = 384;

  std::vector<double> a(inSize), expected(inSize), out(inSize, 0);

  int vRomSize = 32;
  vector<double> vRom(vRomSize);
  vector<int> indptr(inSize);

  for (int i = 0; i < vRomSize; i++) {
    vRom[i] = i;
    indptr[i] = i % 16;
  }

  for(int i = 0; i < inSize; ++i) {
    a[i] = i;
    expected[i] =  a[i] * vRom[indptr[i]];
  }

  std::cout << "Running on DFE." << std::endl;
  fpgaNaive(inSize, &indptr[0], &a[0],  // ins
            &out[0],                          // outs
            &vRom[0]);                        // roms


  /***
      Note that you should always test the output of your DFE
      design against a CPU version to ensure correctness.
  */
  for (int i = 0; i < inSize; i++)
    if (abs(out[i] - expected[i]) > 1E-10) {
      printf("Output from DFE did not match CPU: %d : %f != %f\n",
        i, out[i], expected[i]);
      return 1;
    }

  std::cout << "Test passed!" << std::endl;
  return 0;
}
