# Usage: type 'make help'

# ---- Project settings ----

WHEREISROOT=../../..


# Assuming Kernel and Manager class names follow the pattern below:
PRJ=BiConjugateGradient
KERNEL=$(PRJ)Kernel
MANAGER=$(PRJ)Manager

ENGINEFILES=../$(KERNEL).maxj ../$(MANAGER).maxj
CPUFILES=../$(PRJ).cpp

# Using known hardware settings
include $(WHEREISROOT)/Makefile.Maia.hardware


# Using common compillation targets and compiler settings
include $(WHEREISROOT)/Makefile.common
