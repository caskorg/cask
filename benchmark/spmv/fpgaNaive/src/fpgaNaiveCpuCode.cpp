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

  int fpL = fpgaNaive_fpL;

  const int inSize = fpL * 4;

  std::vector<double> a(inSize), expected(inSize), out(inSize, 0);

  int vRomSize = 32;
  vector<double> vRom(vRomSize);
  vector<int> indptr(inSize);

  for (int i = 0; i < vRomSize; i++) {
    vRom[i] = i;
  }

  for(int i = 0; i < inSize; ++i) {
    indptr[i] = i % vRomSize;
    a[i] = i;
    int prev = i < fpL ? 0 : expected[i - fpL];
    expected[i] =  prev + a[i] * vRom[indptr[i]];
  }

  std::cout << "Running on DFE." << std::endl;
  fpgaNaive(inSize, &indptr[0], &a[0],  // ins
            &out[0],                          // outs
            &vRom[0]);                        // roms


  cout << "cycle\tin\tout\texp" << endl;
  cout << "-----\t--\t---\t---" << endl;
  for (int i = 0; i < inSize + fpL; i++) {
    if ( i >= inSize) {
      cout << i << "\t-" << "\t" << out[i - inSize] << "\t" << expected[i - fpL];
    } else {
      cout << i << "\t" << a[i] * vRom[indptr[i]] << "\t-\t-";
    }
    cout << endl;
  }

  for (int i = 0; i < fpL; i++)
    if (abs(out[i] - expected[inSize - fpL + i]) > 1E-10) {
      printf("Output from DFE did not match CPU: %d : %f != %f\n",
             i, out[i], expected[inSize - fpL + i]);
      return 1;
    }

  std::cout << "Test passed!" << std::endl;
  return 0;
}
