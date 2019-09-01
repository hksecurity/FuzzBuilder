#!/bin/bash -eux
#
# Copyright 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################

if [[ "$#" -ge "1" && $1 == "cov" ]];
then
    CFLAGS_TEST="-m32"
    CXXFLAGS_TEST="-m32"
    CFLAGS="-fprofile-arcs -ftest-coverage -m32"
    CXXFLAGS="-fprofile-arcs -ftest-coverage -m32"
    LDFLAGS="-fprofile-arcs -ftest-coverage -m32 -pthread"
    LDFLAGS_TEST="-fprofile-arcs -ftest-coverage -m32 -pthread"
else
    CFLAGS_TEST="-m32"
    CXXFLAGS_TEST="-m32"
    CFLAGS="-m32"
    CXXFLAGS="-m32"
    LDFLAGS="-m32"
    LDFLAGS_TEST="-m32"
fi

CC=/tool/afl-2.52b/afl-clang
CXX=/tool/afl-2.52b/afl-clang++
nproc=1
SRC=.
OUT=.

CFLAGS="$CFLAGS -DBORINGSSL_UNSAFE_FUZZER_MODE"
CXXFLAGS="$CXXFLAGS -DBORINGSSL_UNSAFE_FUZZER_MODE"
CFLAGS_TEST="$CFLAGS_TEST -DBORINGSSL_UNSAFE_FUZZER_MODE"
CXXFLAGS_TEST="$CXXFLAGS_TEST -DBORINGSSL_UNSAFE_FUZZER_MODE"

CMAKE_DEFINES="-DBORINGSSL_ALLOW_CXX_RUNTIME=1 -DOPENSSL_NO_ASM=1"

cmake -GNinja -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX \
      -DCMAKE_C_FLAGS="$CFLAGS" -DCMAKE_CXX_FLAGS="$LDFLAGS" \
      -DCMAKE_AR=/usr/bin/ar -DCMAKE_EXE_LINKER_FLAGS="$LDFLAGS" \
      -DCMAKE_SHARED_LINKER_FLAGS="$LDFLAGS" -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
      -DCMAKE_SYSTEM_PROCESSOR="i386" \
      -DCMAKE_TOOLCHAIN_FILE=$SRC/util/32-bit-toolchain.cmake \
      $CMAKE_DEFINES $SRC

ninja clean

if [[ "$#" -ge "1" && ( $1 == "cov" || $1 == "seed" ) ]];
then
    ninja
else
    AFL_USE_ASAN=1 ninja
fi

if [[ "$#" -ge "1" && $1 == "seed" ]]
then
    exit
fi

fuzzerFiles=$(find $SRC/fuzz/ -name "*.cc")

$CXX $CXXFLAGS_TEST -std=c++11 -c $SRC/fuzzer_main.cc

for F in $fuzzerFiles; do
    fuzzerName=$(basename $F .cc)
    echo "Building fuzzer $fuzzerName"
    $CXX $CXXFLAGS_TEST -std=c++11 -I$SRC/include -c $F
    AFL_USE_ASAN=1 $CXX $LDFLAGS_TEST -o $OUT/${fuzzerName} $fuzzerName.o fuzzer_main.o \
        ./ssl/libssl.a ./crypto/libcrypto.a

    if [ -d "$SRC/fuzz/${fuzzerName}_corpus" ]; then
        zip -j $OUT/${fuzzerName}_seed_corpus.zip $SRC/fuzz/${fuzzerName}_corpus/*
    fi
done
