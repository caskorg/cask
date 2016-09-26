ifndef PETSC_DIR
  $(error PETSC_DIR is undefined)
endif

CFLAGS	         =
FFLAGS	         =
CPPFLAGS         =
FPPFLAGS         =

include ${PETSC_DIR}/conf/variables
include ${PETSC_DIR}/conf/rules

% : %.c

%: %.o chkopts
	-${CLINKER} -o $@ $<  ${PETSC_KSP_LIB}
