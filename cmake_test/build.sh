#!/bin/bash

set -xeuo pipefail

source /opt/devkitpro/switchvars.sh
toolchain=/opt/devkitpro/switch.cmake

cmake -B build \
	-DCMAKE_TOOLCHAIN_FILE=${toolchain} \
	-DCMAKE_CROSSCOMPILING=1

pushd build
	make
popd
