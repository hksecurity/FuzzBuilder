#!/bin/bash -eu
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
    LDFLAGS="-fprofile-arcs -ftest-coverage -m32 -static"
    LDFLAGS_TEST="-fprofile-arcs -ftest-coverage -m32"
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
OUT=.
SRC=.

./buildconf.sh
./configure CC=$CC CXX=$CXX CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS"

make clean

if [[ "$#" -ge "1" && ( $1 == "cov" || $1 == "seed" ) ]];
then
    make
    make check
else
    AFL_USE_ASAN=1 make
    ASAN_OPTIONS=detect_leaks=0 AFL_USE_ASAN=1 make check
fi

if [[ "$#" -ge "1" && $1 == "seed" ]];
then
    exit
fi

ENCODING_TYPES="UTF_16 \
  UTF_8 \
  ISO_8859_1 \
  US_ASCII \
  UTF_16BE \
  UTF_16LE"

$CXX $CXXFLAGS_TEST -std=c++11 -Ilib/ -c $SRC/StandaloneFuzzTargetMain.c -o $SRC/StandaloneFuzzTargetMain.o

for encoding in $ENCODING_TYPES; do
  fuzz_target_name=parse_${encoding}_fuzzer

  $CXX $CXXFLAGS_TEST -std=c++11 -Ilib/ -DENCODING_${encoding} -c $SRC/parse_fuzzer.cc -o \
    $SRC/parse_${encoding}_fuzzer.o
  AFL_USE_ASAN=1 $CXX $CXXFLAGS_TEST $LDFLAGS_TEST -std=c++11 -Ilib/ -DENCODING_${encoding} \
    -o $OUT/${fuzz_target_name} $SRC/StandaloneFuzzTargetMain.o $SRC/parse_${encoding}_fuzzer.o lib/.libs/libexpat.a
done
