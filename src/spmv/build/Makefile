# Usage: type 'make help'

# ---- Project settings ----

WHEREISROOT=../../..


# Assuming Kernel and Manager class names follow the pattern below:
PRJ=Spmv
KERNEL=$(PRJ)Kernel
MANAGER=$(PRJ)Manager

ENGINEFILES=../src/*.maxj ../src/*.java $(WHEREISROOT)/lib/maxpower/src
CPUFILES=../src/*.cpp

# Using known hardware settings
include $(WHEREISROOT)/Makefile.Maia.hardware


# Using common compillation targets and compiler settings
include $(WHEREISROOT)/Makefile.common
