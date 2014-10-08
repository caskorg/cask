# assume cuda libs are in home directory, if no path set
CUDA_PATH ?= $(shell readlink -f ~/cuda)

all:
	nvcc -O3 cg.cu -o cg -I$(CUDA_PATH)
