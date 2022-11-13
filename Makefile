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

test: build
	make -C "build" test

clean:
	@rm -fR ./build


deb: deb-toolkit deb-binary deb-source deb-clean

deb-toolkit:
	sudo apt install build-essential cmake

deb-binary:
	DEB_BUILD_OPTIONS="nocheck" dpkg-buildpackage -us -uc -b

deb-source:
	dpkg-buildpackage -us -uc -S

deb-clean:
	fakeroot debian/rules clean

rpm: rpm-toolkit rpm-binary rpm-clean

rpm-toolkit:
	sudo dnf install cmake gcc rpm-build rpm-devel rpmlint make python bash coreutils diffutils patch rpmdevtools
	rpmdev-setuptree

rpm-binary:
	rpmbuild -bb --clean --build-in-place ./c-respawner-demo.spec

rpm-clean: clean

all: build

.PHONY: all clean prepare build test
.PHONY: deb deb-toolkit deb-binary deb-source deb-clean
.PHONY: rpm rpm-toolkit rpm-binary rpm-clean

.DEFAULT_GOAL := all
