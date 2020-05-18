# FuzzBuilder: Automated building greybox fuzzing environment for C/C++ library (Extended)

## Introduction
FuzzBuilder is a tool for generating an executable automatically for library fuzzing by using a unit test. Further, FuzzBuilder can generate seed files to fuzz a specific library API function by analyzing a unit test. Generated executables are compatible with various greybox fuzzers like AFL. Using these features, FuzzBuilder can help to apply greybox fuzzing widely on a development process. We will provide source code of FuzzBuilder with detailed information about how to build and how to use. Briefly, FuzzBuilder requires LLVM-6.0, clang-6.0(exactly 6.0.1) packages to be built. FuzzBuilder has beentested on the Linux Debian 4.9.0-8-amd64. Further, the current version of FuzzBuilder can take only 32-bit bitcode files. It is difficult to set up the same experiment environment of this paper. Thus, we will prepare Dockerfile that can generate a docker image to set up every resource for evaluation automatically. Along with Dockerfile, several bash or python scripts will be provided to getting result from a docker container.

## Setup (Docker)
## 1. Docker Image Build
```
sudo docker build -t "fuzzbuilder:v1" .
```

## 2. c-ares Evaluation
### 2-1. FuzzBuilder
```
sudo docker run -it --name c_ares_FB --privileged fuzzbuilder:v1 /bin/bash
```
**Docker Container**
```
# echo core > /proc/sys/kernel/core_pattern
# mkdir -p /exp/c-ares/output/fuzzbuilder
# mkdir -p /exp/c-ares/fc1 && cd /exp/c-ares/fc1 && timeout 21600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/c-ares/seed/fuzzbuilder/opt/ares_create_query -o /exp/c-ares/output/fuzzbuilder/ares_create_query_fuzzer /exp/c-ares/bin/fuzz/fuzzbuilder/ares_create_query_fuzzer
```

* seed coverage & number of seeds
```
# python /tool/coverage.py /exp/c-ares c1 file source/c-ares seed seed_c1_f.txt bin/cov/oss-fuzz/ares_create_query_fuzzer seed/eval/ares_create_query/fuzzbuilder
```

* fuzzing coverage
```
# python /tool/coverage.py /exp/c-ares fc1 stdin source/c-ares exec exec_fc1_f.txt bin/cov/fuzzbuilder/ares_create_query_fuzzer output/fuzzbuilder/ares_create_query_fuzzer/queue
```

### 2-2. OSS-Fuzz
```
sudo docker run -it --name c_ares_OSS --privileged fuzzbuilder:v1 /bin/bash
```
**Docker Container**
```
# echo core > /proc/sys/kernel/core_pattern
# mkdir -p /exp/c-ares/output/oss-fuzz
# mkdir -p /exp/c-ares/c1 && cd /exp/c-ares/c1 && timeout 21600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/c-ares/seed/eval/ares_create_query/oss-fuzz -o /exp/c-ares/output/oss-fuzz/ares_create_query_fuzzer /exp/c-ares/bin/fuzz/oss-fuzz/ares_create_query_fuzzer @@
```

* seed coverage & number of seeds
```
# python /tool/coverage.py /exp/c-ares c1 file source/c-ares seed seed_c1_o.txt bin/cov/oss-fuzz/ares_create_query_fuzzer seed/eval/ares_create_query/oss-fuzz
```

* fuzzing coverage
```
# python /tool/coverage.py /exp/c-ares c1 file source/c-ares exec exec_c1_o.txt bin/cov/oss-fuzz/ares_create_query_fuzzer output/oss-fuzz/ares_create_query_fuzzer/queue
```

## Setup (Manual)

## 1. Environment

- OS : Ubuntu 18.04 Desktop 64 bit
- Compiler : clang-6.0

### 1-1. Ubuntu packages

```bash
apt-get install clang-6.0 clang-6.0-dev llvm-6.0 llvm-6.0-dev
ln -s /usr/bin/clang-6.0 /usr/bin/clang
apt-get install git wget build-essential cmake gcc-multilib g++-multilib gnupg zip autoconf automake libtool docbook2x zlib1g-dev rapidjson-dev
```

### 1-2. AFL setup

```bash
git clone https://github.com/google/AFL.git
cd AFL
make
export AFL_PATH=$PWD
export PATH=$PATH:$PWD
```

- 다음 에러에 대해선 `$ export CC=clang-6.0` 명령어 사용

```jsx
[*] Checking for the ability to compile x86 code...
[+] Everything seems to be working, ready to compile.
cc -O3 -funroll-loops -Wall -D_FORTIFY_SOURCE=2 -g -Wno-pointer-sign -DAFL_PATH=\"/usr/local/lib/afl\" -DDOC_PATH=\"/usr/local/share/doc/afl\" -DBIN_PATH=\"/usr/local/bin\" afl-fuzz.c -o afl-fuzz -ldl
cc -O3 -funroll-loops -Wall -D_FORTIFY_SOURCE=2 -g -Wno-pointer-sign -DAFL_PATH=\"/usr/local/lib/afl\" -DDOC_PATH=\"/usr/local/share/doc/afl\" -DBIN_PATH=\"/usr/local/bin\" afl-tmin.c -o afl-tmin -ldl
cc -O3 -funroll-loops -Wall -D_FORTIFY_SOURCE=2 -g -Wno-pointer-sign -DAFL_PATH=\"/usr/local/lib/afl\" -DDOC_PATH=\"/usr/local/share/doc/afl\" -DBIN_PATH=\"/usr/local/bin\" afl-gotcpu.c -o afl-gotcpu -ldl
cc -O3 -funroll-loops -Wall -D_FORTIFY_SOURCE=2 -g -Wno-pointer-sign -DAFL_PATH=\"/usr/local/lib/afl\" -DDOC_PATH=\"/usr/local/share/doc/afl\" -DBIN_PATH=\"/usr/local/bin\" afl-analyze.c -o afl-analyze -ldl
[*] Testing the CC wrapper and instrumentation output...
unset AFL_USE_ASAN AFL_USE_MSAN; AFL_QUIET=1 AFL_INST_RATIO=100 AFL_PATH=. ./afl-gcc -O3 -funroll-loops -Wall -D_FORTIFY_SOURCE=2 -g -Wno-pointer-sign -DAFL_PATH=\"/usr/local/lib/afl\" -DDOC_PATH=\"/usr/local/share/doc/afl\" -DBIN_PATH=\"/usr/local/bin\" test-instr.c -o test-instr -ldl
./afl-showmap -m none -q -o .test-instr0 ./test-instr < /dev/null
echo 1 | ./afl-showmap -m none -q -o .test-instr1 ./test-instr

Oops, the instrumentation does not seem to be behaving correctly!

Please ping <lcamtuf@google.com> to troubleshoot the issue.

Makefile:90: recipe for target 'test_build' failed
make: *** [test_build] Error 1
```

### 1-3. Build for FuzzBuilder

```bash
git clone https://github.com/hksecurity/FuzzBuilder.git
cd FuzzBuilder
export FuzzBuilder=$PWD
mkdir -p $FuzzBuilder/build && cd $FuzzBuilder/build
cmake build ../src
make
```

## 2. Example for `expat` Project

### 2-1. Download source code of `expat` project

```bash
mkdir -p $FuzzBuilder/exp/expat/source && cd $FuzzBuilder/exp/expat/source
git clone [https://github.com/libexpat/libexpat](https://github.com/libexpat/libexpat) && cd libexpat
git checkout 39e487da353b20bb3a724311d179ba0fddffc65b
cp $FuzzBuilder/exp/expat/fuzzer_main.cc ./expat
cp $FuzzBuilder/exp/expat/build.sh ./expat
```

### 2-2. Build the project

```bash
$ cd $FuzzBuilder/exp/expat/source/libexpat/expat
# edit CC, CXX variable with correct path
$ vi ./build.sh
$ ./build.sh seed
```

### 2-3. Applying FuzzBuilder to generate seeds

```bash
$ afl-clang -emit-llvm -DHAVE_CONFIG_H -I. -I.. -DHAVE_EXPAT_CONFIG_H -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -c lib/xmlparse.c -fPIC -DPIC
$ $FuzzBuilder/build/fuzzbuilder seed $FuzzBuilder/exp/expat/seed.conf
```

### 2-4. Build a unit test with instrumented bitcode file

```bash
afl-clang -DHAVE_CONFIG_H -I. -I.. -DHAVE_EXPAT_CONFIG_H -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -c xmlparse.bc.mod.bc -fPIC -DPIC -o xmlparse.o
ar r lib/.libs/libexpat.a xmlparse.o
rm -f tests/runtests
make check
```

### 2-5. Run a unit test to collect seed files

```bash
mv /tmp/fuzzbuilder.collect .
python $FuzzBuilder/script/seed_maker.py fuzzbuilder.collect seed_fb
mkdir -p $FuzzBuilder/exp/expat/seed/fuzzbuilder
mv $FuzzBuilder/exp/expat/source/libexpat/expat/seed_fb $FuzzBuilder/exp/expat/seed/fuzzbuilder/org
```

### 2-6. FuzzBuilder to generate fuzzing executables
ASAN 실행파일 에러 해결 필요

```bash
cd $FuzzBuilder/exp/expat/source/libexpat/expat/tests
afl-clang -emit-llvm -DHAVE_CONFIG_H -I. -I.. -I./../lib -DHAVE_EXPAT_CONFIG_H -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -c runtests.c
$FuzzBuilder/build/fuzzbuilder exec $FuzzBuilder/exp/expat/XML_Parse.conf
afl-clang -DHAVE_CONFIG_H -I. -I.. -I./../lib -DHAVE_EXPAT_CONFIG_H -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -o runtests.o -c runtests.bc.mod.bc
#AFL_USE_ASAN=1 afl-clang -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -fno-strict-aliasing -o XML_Parse_fuzzer runtests.o libruntests.a ../lib/.libs/libexpat.a
afl-clang -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -fno-strict-aliasing -o XML_Parse_fuzzer runtests.o libruntests.a ../lib/.libs/libexpat.a
mkdir -p $FuzzBuilder/exp/expat/bin/fuzz/fuzzbuilder
mv XML_Parse_fuzzer $FuzzBuilder/exp/expat/bin/fuzz/fuzzbuilder

$FuzzBuilder/build/fuzzbuilder exec $FuzzBuilder/exp/expat/bug.conf
afl-clang -DHAVE_CONFIG_H -I. -I.. -I./../lib -DHAVE_EXPAT_CONFIG_H -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -o runtests.o -c runtests.bc.mod.bc
#AFL_USE_ASAN=1 afl-clang -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -fno-strict-aliasing -o XML_Parse_fuzzer runtests.o libruntests.a ../lib/.libs/libexpat.a
afl-clang -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -fno-strict-aliasing -o XML_Parse_fuzzer runtests.o libruntests.a ../lib/.libs/libexpat.a
mv XML_Parse_fuzzer $FuzzBuilder/exp/expat/bin/fuzz/fuzzbuilder/bug_fuzzer
```

### 2-7. FuzzBuilder to generate coverage executables

```bash
cd $FuzzBuilder/exp/expat/source/libexpat/expat/tests
afl-clang -emit-llvm -DHAVE_CONFIG_H -I. -I.. -I./../lib -DHAVE_EXPAT_CONFIG_H -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -c runtests.c
$FuzzBuilder/build/fuzzbuilder exec $FuzzBuilder/exp/expat/XML_Parse.conf
afl-clang -DHAVE_CONFIG_H -I. -I.. -I./../lib -DHAVE_EXPAT_CONFIG_H -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -o runtests.o -c runtests.bc.mod.bc
afl-clang -fprofile-arcs -ftest-coverage -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -m32 -g -fno-strict-aliasing -o XML_Parse_fuzzer runtests.o  libruntests.a ../lib/.libs/libexpat.a
mkdir -p $FuzzBuilder/exp/expat/bin/cov/fuzzbuilder
mv XML_Parse_fuzzer $FuzzBuilder/exp/expat/bin/cov/fuzzbuilder
```

### 2-8. Seed Optimization (afl-cmin)

```bash
cd $FuzzBuilder/exp/expat
afl-cmin -m 1024 -i seed/fuzzbuilder/org/XML_Parse -o seed/eval/XML_Parse_fuzzer/fuzzbuilder bin/fuzz/fuzzbuilder/XML_Parse_fuzzer @@
```

- 3727개의 시드파일에 대해서 실행결과 `[!] WARNING: All test cases had the same traces, check syntax!` 메시지와 함께 1개의 시드파일만 생성됨

### 2-9. Fuzzing

Raw seed 생성 점검 필요 (실행하자마자 바로 크래시 발견)

```bash
cd $FuzzBuilder
mkdir -p exp/expat/output/fuzzbuilder/XML_Parse_Fuzzer
# Raw seeds
afl-fuzz -m 1024 -i exp/expat/seed/fuzzbuilder/org/XML_Parse/ -o exp/expat/output/fuzzbuilder/XML_Parse_Fuzzer exp/expat/bin/fuzz/fuzzbuilder/XML_Parse_fuzzer
# Optimized seeds (afl-cmin)
afl-fuzz -m 1024 -i exp/expat/seed/eval/XML_Parse_fuzzer/fuzzbuilder -o exp/expat/output/fuzzbuilder/XML_Parse_Fuzzer exp/expat/bin/fuzz/fuzzbuilder/XML_Parse_fuzzer
```

## 3. Evalucation

### 3-1. Docker Container
**c-ares**
```bash
docker run -it --name "c_ares_fc1" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "c_ares_fc2" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "c_ares_c1" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "c_ares_c2" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "c_ares_c1s" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "c_ares_c2s" "fuzzbuilderex:v1" /bin/bash
```

**expat**
```bash
docker run -it --name "expat_fe1" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "expat_e1" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "expat_e2" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "expat_e3" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "expat_e4" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "expat_e5" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "expat_e6" "fuzzbuilderex:v1" /bin/bash
```

**boringssl**
```bash
docker run -it --name "boring_fb1" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "boring_fb2" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "boring_fb3" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "boring_fb4" "fuzzbuilderex:v1" /bin/bash
docker run -it --name "boring_b1" "fuzzbuilderex:v1" /bin/bash
```

### 3-2. Generate output directories
**c-ares**
```bash
mkdir -p /exp/c-ares/output/fuzzbuilder && mkdir -p /exp/c-ares/output/oss-fuzz
```

**expat**
```bash
mkdir -p /exp/expat/output/fuzzbuilder && mkdir -p /exp/expat/output/oss-fuzz
```

**boringssl**
```bash
mkdir -p /exp/boringssl/output/fuzzbuilder && mkdir -p /exp/boringssl/output/oss-fuzz
```

### 3-3. Fuzzing
**c-ares**
```bash
# different seed  
fc1: mkdir -p /exp/c-ares/fc1 && cd /exp/c-ares/fc1 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/c-ares/seed/fuzzbuilder/opt/ares_create_query -o /exp/c-ares/output/fuzzbuilder/ares_create_query_fuzzer /exp/c-ares/bin/fuzz/fuzzbuilder/ares_create_query_fuzzer
fc2: mkdir -p /exp/c-ares/fc2 && cd /exp/c-ares/fc2 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/c-ares/seed/fuzzbuilder/opt/ares_parse_reply -o /exp/c-ares/output/fuzzbuilder/ares_parse_reply_fuzzer /exp/c-ares/bin/fuzz/fuzzbuilder/ares_parse_reply_fuzzer
c1: mkdir -p /exp/c-ares/c1 && cd /exp/c-ares/c1 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/c-ares/seed/eval/ares_create_query/oss-fuzz -o /exp/c-ares/output/oss-fuzz/ares_create_query_fuzzer /exp/c-ares/bin/fuzz/oss-fuzz/ares_create_query_fuzzer @@
c2: mkdir -p /exp/c-ares/c2 && cd /exp/c-ares/c2 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/c-ares/seed/eval/ares_parse_reply/oss-fuzz -o /exp/c-ares/output/oss-fuzz/ares_parse_reply_fuzzer /exp/c-ares/bin/fuzz/oss-fuzz/ares_parse_reply_fuzzer @@
# same seed
c1s: mkdir -p /exp/c-ares/c1 && cd /exp/c-ares/c1 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/c-ares/seed/fuzzbuilder/opt/ares_create_query -o /exp/c-ares/output/oss-fuzz/ares_create_query_fuzzer_same /exp/c-ares/bin/fuzz/oss-fuzz/ares_create_query_fuzzer @@
c2s: mkdir -p /exp/c-ares/c2 && cd /exp/c-ares/c2 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/c-ares/seed/fuzzbuilder/opt/ares_parse_reply -o /exp/c-ares/output/oss-fuzz/ares_parse_reply_fuzzer_same /exp/c-ares/bin/fuzz/oss-fuzz/ares_parse_reply_fuzzer @@
```

**expat**
```bash
# different seed
fe1: mkdir -p /exp/expat/fe1 && cd /exp/expat/fe1 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/fuzzbuilder/opt/XML_Parse -o /exp/expat/output/fuzzbuilder/XML_Parse_fuzzer /exp/expat/bin/fuzz/fuzzbuilder/XML_Parse_fuzzer
e1: mkdir -p /exp/expat/e1 && cd /exp/expat/e1 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/eval/parse_ISO_8859_1_fuzzer/oss-fuzz -o /exp/expat/output/oss-fuzz/parse_ISO_8859_1_fuzzer /exp/expat/bin/fuzz/oss-fuzz/parse_ISO_8859_1_fuzzer @@
e2: mkdir -p /exp/expat/e2 && cd /exp/expat/e2 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/eval/parse_US_ASCII_fuzzer/oss-fuzz -o /exp/expat/output/oss-fuzz/parse_US_ASCII_fuzzer /exp/expat/bin/fuzz/oss-fuzz/parse_US_ASCII_fuzzer @@
e3: mkdir -p /exp/expat/e3 && cd /exp/expat/e3 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/eval/parse_UTF_16BE_fuzzer/oss-fuzz -o /exp/expat/output/oss-fuzz/parse_UTF_16BE_fuzzer /exp/expat/bin/fuzz/oss-fuzz/parse_UTF_16BE_fuzzer @@
e4: mkdir -p /exp/expat/e4 && cd /exp/expat/e4 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/eval/parse_UTF_16_fuzzer/oss-fuzz -o /exp/expat/output/oss-fuzz/parse_UTF_16_fuzzer /exp/expat/bin/fuzz/oss-fuzz/parse_UTF_16_fuzzer @@
e5: mkdir -p /exp/expat/e5 && cd /exp/expat/e5 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/eval/parse_UTF_16LE_fuzzer/oss-fuzz -o /exp/expat/output/oss-fuzz/parse_UTF_16LE_fuzzer /exp/expat/bin/fuzz/oss-fuzz/parse_UTF_16LE_fuzzer @@
e6: mkdir -p /exp/expat/e6 && cd /exp/expat/e6 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/eval/parse_UTF_8_fuzzer/oss-fuzz -o /exp/expat/output/oss-fuzz/parse_UTF_8_fuzzer /exp/expat/bin/fuzz/oss-fuzz/parse_UTF_8_fuzzer @@
# same seed
e1s: mkdir -p /exp/expat/e1 && cd /exp/expat/e1 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/fuzzbuilder/opt/XML_Parse -o /exp/expat/output/oss-fuzz/parse_ISO_8859_1_fuzzer_same /exp/expat/bin/fuzz/oss-fuzz/parse_ISO_8859_1_fuzzer @@
e2s: mkdir -p /exp/expat/e2 && cd /exp/expat/e2 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/fuzzbuilder/opt/XML_Parse -o /exp/expat/output/oss-fuzz/parse_US_ASCII_fuzzer_same /exp/expat/bin/fuzz/oss-fuzz/parse_US_ASCII_fuzzer @@
e3s: mkdir -p /exp/expat/e3 && cd /exp/expat/e3 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/fuzzbuilder/opt/XML_Parse -o /exp/expat/output/oss-fuzz/parse_UTF_16BE_fuzzer_same /exp/expat/bin/fuzz/oss-fuzz/parse_UTF_16BE_fuzzer @@
e4s: mkdir -p /exp/expat/e4 && cd /exp/expat/e4 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/fuzzbuilder/opt/XML_Parse -o /exp/expat/output/oss-fuzz/parse_UTF_16_fuzzer_same /exp/expat/bin/fuzz/oss-fuzz/parse_UTF_16_fuzzer @@
e5s: mkdir -p /exp/expat/e5 && cd /exp/expat/e5 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/fuzzbuilder/opt/XML_Parse -o /exp/expat/output/oss-fuzz/parse_UTF_16LE_fuzzer_same /exp/expat/bin/fuzz/oss-fuzz/parse_UTF_16LE_fuzzer @@
e6s: mkdir -p /exp/expat/e6 && cd /exp/expat/e6 && timeout 3600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/expat/seed/fuzzbuilder/opt/XML_Parse -o /exp/expat/output/oss-fuzz/parse_UTF_8_fuzzer_same /exp/expat/bin/fuzz/oss-fuzz/parse_UTF_8_fuzzer @@
```

### 3-4. Coverage
**c-ares**
```bash
# different seed  
fc1: python /tool/coverage.py /exp/c-ares fc1 stdin source/c-ares exec exec_fc1_f.txt bin/cov/fuzzbuilder/ares_create_query_fuzzer output/fuzzbuilder/ares_create_query_fuzzer/queue
fc2: python /tool/coverage.py /exp/c-ares fc2 stdin source/c-ares exec exec_fc2_f.txt bin/cov/fuzzbuilder/ares_parse_reply_fuzzer output/fuzzbuilder/ares_parse_reply_fuzzer/queue
c1: python /tool/coverage.py /exp/c-ares c1 file source/c-ares exec exec_c1_o.txt bin/cov/oss-fuzz/ares_create_query_fuzzer output/oss-fuzz/ares_create_query_fuzzer/queue
c2: python /tool/coverage.py /exp/c-ares c2 file source/c-ares exec exec_c2_o.txt bin/cov/oss-fuzz/ares_parse_reply_fuzzer output/oss-fuzz/ares_parse_reply_fuzzer/queue
# same seed
c1s: python /tool/coverage.py /exp/c-ares c1 file source/c-ares exec exec_c1s_o.txt bin/cov/oss-fuzz/ares_create_query_fuzzer output/oss-fuzz/ares_create_query_fuzzer_same/queue
c2s: python /tool/coverage.py /exp/c-ares c2 file source/c-ares exec exec_c2s_o.txt bin/cov/oss-fuzz/ares_parse_reply_fuzzer output/oss-fuzz/ares_parse_reply_fuzzer_same/queue
```

**expat**
```bash
# different seed 
fe1: python /tool/coverage.py /exp/expat fe1 stdin source/libexpat/expat exec exec_fe1_f.txt bin/cov/fuzzbuilder/XML_Parse_fuzzer output/fuzzbuilder/XML_Parse_fuzzer/queue
e1: python /tool/coverage.py /exp/expat e1 file source/libexpat/expat exec exec_e1_o.txt bin/cov/oss-fuzz/parse_ISO_8859_1_fuzzer output/oss-fuzz/parse_ISO_8859_1_fuzzer/queue
e2: python /tool/coverage.py /exp/expat e2 file source/libexpat/expat exec exec_e2_o.txt bin/cov/oss-fuzz/parse_US_ASCII_fuzzer output/oss-fuzz/parse_US_ASCII_fuzzer/queue
e3: python /tool/coverage.py /exp/expat e3 file source/libexpat/expat exec exec_e3_o.txt bin/cov/oss-fuzz/parse_UTF_16BE_fuzzer output/oss-fuzz/parse_UTF_16BE_fuzzer/queue
e4: python /tool/coverage.py /exp/expat e4 file source/libexpat/expat exec exec_e4_o.txt bin/cov/oss-fuzz/parse_UTF_16_fuzzer output/oss-fuzz/parse_UTF_16_fuzzer/queue
e5: python /tool/coverage.py /exp/expat e5 file source/libexpat/expat exec exec_e5_o.txt bin/cov/oss-fuzz/parse_UTF_16LE_fuzzer output/oss-fuzz/parse_UTF_16LE_fuzzer/queue
e6: python /tool/coverage.py /exp/expat e6 file source/libexpat/expat exec exec_e6_o.txt bin/cov/oss-fuzz/parse_UTF_8_fuzzer output/oss-fuzz/parse_UTF_8_fuzzer/queue
# same seed
e1s: python /tool/coverage.py /exp/expat e1 file source/libexpat/expat exec exec_e1s_o.txt bin/cov/oss-fuzz/parse_ISO_8859_1_fuzzer output/oss-fuzz/parse_ISO_8859_1_fuzzer_same/queue
e2s: python /tool/coverage.py /exp/expat e2 file source/libexpat/expat exec exec_e2s_o.txt bin/cov/oss-fuzz/parse_US_ASCII_fuzzer output/oss-fuzz/parse_US_ASCII_fuzzer_same/queue
e3s: python /tool/coverage.py /exp/expat e3 file source/libexpat/expat exec exec_e3s_o.txt bin/cov/oss-fuzz/parse_UTF_16BE_fuzzer output/oss-fuzz/parse_UTF_16BE_fuzzer_same/queue
e4s: python /tool/coverage.py /exp/expat e4 file source/libexpat/expat exec exec_e4s_o.txt bin/cov/oss-fuzz/parse_UTF_16_fuzzer output/oss-fuzz/parse_UTF_16_fuzzer_same/queue
e5s: python /tool/coverage.py /exp/expat e5 file source/libexpat/expat exec exec_e5s_o.txt bin/cov/oss-fuzz/parse_UTF_16LE_fuzzer output/oss-fuzz/parse_UTF_16LE_fuzzer_same/queue
e6s: python /tool/coverage.py /exp/expat e6 file source/libexpat/expat exec exec_e6s_o.txt bin/cov/oss-fuzz/parse_UTF_8_fuzzer output/oss-fuzz/parse_UTF_8_fuzzer_same/queue
```

## Publications

```
FuzzBuilder: automated building greybox fuzzing environment for C/C++ library

@inproceedings{jang2019fuzzbuilder,
  title={FuzzBuilder: automated building greybox fuzzing environment for C/C++ library},
  author={Jang, Joonun and Kim, Huy Kang},
  booktitle={Proceedings of the 35th Annual Computer Security Applications Conference},
  pages={627--637},
  year={2019}
}
```
