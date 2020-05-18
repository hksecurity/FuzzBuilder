#!/bin/bash -eu
# CFLAGS="-fsanitize=fuzzer-no-link,address -Os"
# CXXFLAGS="-fsanitize=fuzzer-no-link,address -Os"
CFLAGS="-fsanitize=fuzzer-no-link,address -fprofile-arcs -ftest-coverage -Os"
CXXFLAGS="-fsanitize=fuzzer-no-link,address -fprofile-arcs -ftest-coverage -Os"
CC=/usr/bin/clang
CXX=/usr/bin/clang++

./buildconf
./configure CC=$CC CXX=$CXX CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS"
make clean

make -j$(nproc) V=1 all

pushd test

make clean
# Build the fuzzers
$CC -I.. -fsanitize-coverage=edge,trace-pc-guard,indirect-calls,trace-cmp,trace-div,trace-gep -c ares-test-fuzz.c
# $CXX -fsanitize=fuzzer,address ares-test-fuzz.o ../.libs/libcares.a -o ares_parse_reply_libfuzzer
$CXX -fsanitize=fuzzer,address --coverage ares-test-fuzz.o ../.libs/libcares.a -o ares_parse_reply_libfuzzer

$CC -I.. -fsanitize-coverage=edge,trace-pc-guard,indirect-calls,trace-cmp,trace-div,trace-gep -c ares-test-fuzz-name.c
# $CXX -fsanitize=fuzzer,address ares-test-fuzz-name.o ../.libs/libcares.a -o ares_create_query_libfuzzer
$CXX -fsanitize=fuzzer,address --coverage ares-test-fuzz-name.o ../.libs/libcares.a -o ares_create_query_libfuzzer

popd