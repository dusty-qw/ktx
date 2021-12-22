#!/bin/bash

# Useful if you willing to stop on first error, also print what is executed.
#set -ex

BUILDIR="${BUILDIR:-build}" # Default build dir.

# Default bot support, CMake already have it as ON,
# provided here just for possibility to turn it OFF without modification of CMake or this script file.
BOT_SUPPORT="${BOT_SUPPORT:-ON}"

# If V variable is not empty then provide -v argument to Ninja (verbose output).
V="${V:-}"
[ ! -z ${V} ] && V="-v"

rm -rf ${BUILDIR}
mkdir -p ${BUILDIR}

# Define target platforms, feel free to comment out if you does not require some of it.
BUILD_LIST=(
	linux-amd64
	linux-aarch64
	linux-armhf
	linux-i686
	windows-x64
	windows-x86
	qvm
)

# Either use default CMake generator (most of the time its make) or ninja if found.
CMAKE_GENERATOR=
if hash ninja >/dev/null 2>&1
then
	CMAKE_GENERATOR="-G Ninja"
fi

# Build platforms one by one.
for name in "${BUILD_LIST[@]}"; do
	mkdir -p ${BUILDIR}/$name
	case "$name" in
	"qvm" ) # Build QVM library.
		cmake -B ${BUILDIR}/$name -S . -DBOT_SUPPORT=${BOT_SUPPORT} ${CMAKE_GENERATOR}
		cmake --build ${BUILDIR}/$name --target qvm ${V}
	;;
	* ) # Build native library.
		cmake -B ${BUILDIR}/$name -S . -DBOT_SUPPORT=${BOT_SUPPORT} -DCMAKE_BUILD_TYPE=Release ${CMAKE_GENERATOR} -DCMAKE_TOOLCHAIN_FILE=tools/cross-cmake/$name.cmake
		cmake --build ${BUILDIR}/$name ${V}
	;;
	esac
done
