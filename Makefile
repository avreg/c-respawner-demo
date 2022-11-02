NPROCS := 1
OS := $(shell uname -s)

ifeq ($(OS),Linux)
	NPROCS := $(shell nproc)
endif

prepare:
	@cmake -S . -B "build" \
    	-DCMAKE_BUILD_TYPE=Debug

build: prepare
	@cmake --build "build" -j $(NPROCS)

clean:
	@rm -fR ./build

all: build

.PHONY: all clean prepare build
.DEFAULT_GOAL := all
