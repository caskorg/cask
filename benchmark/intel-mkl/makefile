ifndef MKL_ROOT
 $(error "MKL_ROOT not set")
endif

# icc
CC=icc
CC_FLAGS = -w

MKL_PATH = $(MKL_ROOT)/lib/intel64
CMPLR_PATH = $(MKL_ROOT)/../compiler/lib/intel64

# All the required libraries
MKL_LIB=$(MKL_PATH)/lib
IFACE_LIB=$(MKL_LIB)mkl_intel_lp64.a
THREADING_LIB=$(MKL_LIB)mkl_intel_thread.a
OMP_LIB = -L"$(CMPLR_PATH)" -liomp5
CORE_LIB=$(MKL_LIB)mkl_core.a

cg:
	$(CC) -m64 $(CC_FLAGS) -I"$(MKL_ROOT)/include" \
	$@.c  $(IFACE_LIB) $(THREADING_LIB) $(CORE_LIB) $(OMP_LIB) -lpthread -lm -o $@.out
	@ export LD_LIBRARY_PATH="$(MKL_PATH)":$(LD_LIBRARY_PATH):$(CMPLR_PATH); \
	time ./$@.out

#-------------------------------------------------------------------------------
clean:
	rm *.out
