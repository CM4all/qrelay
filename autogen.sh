#!/bin/sh

set -e

rm -rf config.cache build
mkdir build
aclocal
automake --add-missing --foreign
autoconf
./configure \
	--prefix=/usr/local/stow/cm4all-qrelay \
	--enable-debug \
	--enable-silent-rules \
	CXX=clang++ \
	CXXFLAGS="-O0 -ggdb" \
	"$@"
