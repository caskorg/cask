ifndef MKLROOT
 $(error "MKLROOT not set")
endif

# Use Intel C Compiler
CC=icc
CC_FLAGS ?= -Wall

MKL_PATH = $(MKLROOT)/lib/intel64
CMPLR_PATH = $(MKLROOT)/../compiler/lib/intel64

# All the required libraries
MKL_LIB=$(MKL_PATH)/lib
IFACE_LIB=$(MKL_LIB)mkl_intel_lp64.a
THREADING_LIB=$(MKL_LIB)mkl_intel_thread.a
OMP_LIB = -L"$(CMPLR_PATH)" -liomp5
CORE_LIB=$(MKL_LIB)mkl_core.a

COMMON_LIB=../lib

MAT ?= sts4098

lib:
	@ $(CC) -O3 -m64 $(CC_FLAGS) -c $(COMMON_LIB)/*.c

cg: lib
	@ $(CC) -O3 -m64 $(CC_FLAGS) -I"$(MKLROOT)/include" -I"$(COMMON_LIB)" -c $@.c
	@ $(CC) $@.o $(COMMON_LIB)/*.o $(IFACE_LIB) $(THREADING_LIB) $(CORE_LIB) $(OMP_LIB) -lpthread -lm -o $@.out
run:
	@ export LD_LIBRARY_PATH="$(MKL_PATH)":$(LD_LIBRARY_PATH):$(CMPLR_PATH); \
	./cg.out $(MAT).mtx $(MAT)_b.mtx $(MM)

#-------------------------------------------------------------------------------
clean:
	rm *.out
