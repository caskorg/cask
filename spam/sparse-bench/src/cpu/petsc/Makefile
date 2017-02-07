ifndef PETSC_DIR
  $(error PETSC_DIR is undefined)
endif

CFLAGS	         =-O3
FFLAGS	         =
CPPFLAGS         =-O3
FPPFLAGS         =

ifeq ($(CXX), icpc)
	CFLAGS	         =-O3 -xHost
	FFLAGS	         =
	CPPFLAGS         =-O3 -xHost
	FPPFLAGS         =
endif

include ${PETSC_DIR}/lib/petsc/conf/variables
include ${PETSC_DIR}/lib/petsc/conf/rules

% : %.c

%: %.o chkopts
	-${CLINKER} -o $@ $<  ${PETSC_KSP_LIB}
