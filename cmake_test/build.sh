#!/bin/bash

set -xeuo pipefail

# source /opt/devkitpro/switchvars.sh
toolchain=/opt/devkitpro/switch.cmake

cmake -B build \
	-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
	-DCMAKE_TOOLCHAIN_FILE=${toolchain} 

pushd build
	make
popd
