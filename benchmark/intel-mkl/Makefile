ifndef MKLROOT
 $(error "MKLROOT not set")
endif

# icc
CC=icc
CC_FLAGS = -w

MKL_PATH = $(MKLROOT)/lib/intel64
CMPLR_PATH = $(MKLROOT)/../compiler/lib/intel64

# All the required libraries
MKL_LIB=$(MKL_PATH)/lib
IFACE_LIB=$(MKL_LIB)mkl_intel_lp64.a
THREADING_LIB=$(MKL_LIB)mkl_intel_thread.a
OMP_LIB = -L"$(CMPLR_PATH)" -liomp5
CORE_LIB=$(MKL_LIB)mkl_core.a

MAT=sts4098
MAT=nasa1824
# MAT=nasa2146
# MAT=nasa2910
MAT = test3

cg:
	$(CC) -O3 -m64 $(CC_FLAGS) -I"$(MKLROOT)/include" \
	$@.c  $(IFACE_LIB) $(THREADING_LIB) $(CORE_LIB) $(OMP_LIB) -lpthread -lm -o $@.out
	@ export LD_LIBRARY_PATH="$(MKL_PATH)":$(LD_LIBRARY_PATH):$(CMPLR_PATH); \
	time ./$@.out $(MAT).mtx $(MAT)_b.mtx $(MM)

#-------------------------------------------------------------------------------
clean:
	rm *.out
