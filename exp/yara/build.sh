#!/bin/bash -eu
# Copyright 2017 Google Inc.
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

if [ "$#" -ge "1" ] && [ $1 == "cov" ];
then
    CFLAGS_TEST="-m32"
    CXXFLAGS_TEST="-m32"
    CFLAGS="-m32 -fprofile-arcs -ftest-coverage"
    CXXFLAGS="-m32 -fprofile-arcs -ftest-coverage"
    LDFLAGS="-m32 -fprofile-arcs -ftest-coverage"
    LDFLAGS_TEST="-m32 -fprofile-arcs -ftest-coverage"
else
    CFLAGS_TEST="-m32"
    CXXFLAGS_TEST="-m32"
    CFLAGS="-m32"
    CXXFLAGS="-m32"
    LDFLAGS="-m32 -static"
    LDFLAGS_TEST="-m32"
fi

CC=/tool/afl-2.52b/afl-clang
CXX=/tool/afl-2.52b/afl-clang++
nproc=1
SRC=.
OUT=.

./bootstrap.sh
./configure CC=$CC CXX=$CXX CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" --enable-debug --enable-dex --enable-dotnet --without-crypto

make clean

if [[ "$#" -ge "1" && ( $1 == "cov" || $1 == "seed" ) ]]
then
    make
    make check || set ?=0
else
    AFL_USE_ASAN=1 make
    ASAN_OPTIONS=detect_leaks=0 AFL_USE_ASAN=1 make check || set ?=0
fi

if [[ "$#" -ge "1" && ( $1 == "seed" ) ]]
then
    exit
fi

$CXX $CXXFLAGS_TEST -std=c++11 -Ilib/ -c $SRC/fuzzer_main.cc -o $SRC/fuzzer_main.o

fuzzers=$(find $SRC/tests/oss-fuzz/ -name "*.cc")
for f in $fuzzers; do
    fuzzer_name=$(basename -s ".cc" $f)
    echo "Building $fuzzer_name"

    $CXX $CXXFLAGS_TEST -std=c++11 -I. -Ilibyara/include -c $f
    AFL_USE_ASAN=1 $CXX $CXXFLAGS_TEST $LDFLAGS_TEST -std=c++11 -o $OUT/$fuzzer_name \
        $fuzzer_name.o $SRC/fuzzer_main.o ./libyara/.libs/libyara.a -pthread

    if [ -d "$SRC/tests/oss-fuzz/${fuzzer_name}_corpus" ]; then
        zip -j $OUT/${fuzzer_name}_seed_corpus.zip $SRC/tests/oss-fuzz/${fuzzer_name}_corpus/*
    fi
done
