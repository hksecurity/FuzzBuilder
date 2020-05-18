Title - FuzzBuilder: Automated building greybox fuzzing environment for C/C++ library

Authors - Joonun Jang(Samsung Research), Huy Kang Kim(Korea University)

Conference - 2019 ACSAC(https://www.acsac.org)

1) Introduction

FuzzBuilder is a tool for generating an executable automatically for library fuzzing by using a unit test. Further, FuzzBuilder can generate seed files to fuzz a specific library API function by analyzing a unit test. Generated executables are compatible with various greybox fuzzers like AFL. Using these features, FuzzBuilder can help to apply greybox fuzzing widely on a development process.
We will provide source code of FuzzBuilder with detailed information about how to build and how to use. Briefly, FuzzBuilder requires LLVM-6.0, clang-6.0(exactly 6.0.1) packages to be built. FuzzBuilder has beentested on the Linux Debian 4.9.0-8-amd64. Further, the current version of FuzzBuilder can take only 32-bit bitcode files.
It is difficult to set up the same experiment environment of this paper. Thus, we will prepare Dockerfile that can generate a docker image to set up every resource for evaluation automatically. Along with Dockerfile, several bash or python scripts will be provided to getting result from a docker container.

2) Environment

Firstly, we will inform you how to install dependencies of FuzzBuilder.

(1) Write below URL into apt sources.list.

deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-6.0 main\

deb-src http://apt.llvm.org/stretch/ llvm-toolchain-stretch-6.0 main

(2) Add gpg key of LLVM 

wget -O ./key.gpg https://apt.llvm.org/llvm-snapshot.gpg.key

apt-key add < ./key.gpg

rm ./key.gpg

(3) Install LLVM and clang through apt-get

apt-get -y update

apt-get -y install clang-6.0 clang-6.0-dev llvm-6.0 llvm-6.0-dev

(optional)

ln -s /usr/bin/clang-6.0 /usr/bin/clang

ln -s /usr/bin/clang++-6.0 /usr/bin/clang++

(4) Install other packages

apt-get install git wget build-essential cmake gcc-multilib g++-multiblib gnupg zip autoconf automake libtool docbook2x zlib1g-dev rapidjson-dev

(5) Build FuzzBuilder 

mkdir -p ${FuzzBuilder repo}/build

cd ${FuzzBuilder repo}/build

cmake build ..

make

Now, you have fuzzbuilder in ${FuzzBuilder repo}/build.

3) How to use FuzzBuilder

FuzzBuilder gets configuration file and generates instrumented bitcode files based on a configuration file. Generated bitcode files have the same name as an original one with post-fix, “.mod.bc”. For example, if an original bitcode file name is “example.bc” then, the output will be “example.bc.mod.bc”.
 
(1) Command-Line Options
 
Command line option for FuzzBuilder is the following.

./fuzzbuilder [seed|exec] ${config file}
 
The first parameter should be seed, or exec.
“seed” means that FuzzBuilder will instrument bitcode files to generate seed files.
“exec” means that FuzzBuilder will instrument bitcode files to generate fuzz-able executable.
Otherwise, ${config file} is a path of a configuration file.

(2) Configuration file
 
The configuration file has 4 options: targets, files, tests, skips.
“targets” is for specifying FA(Fuzzable API) that is used to pass input from a fuzzer to a library. To specify FA, its name, indices of parameters that are used to pass a buffer address and its size if FA requires it.
“files” is for specifying bitcode files that need to be instrumented. Paths of a bitcode file should be specified. If a path of the bitcode file was specified as a relative path, then FuzzBuilder finds it based on the current working directory.
“tests” is for specifying test function names need to be analyzed. Currently, this test function should have void type as a return type to ensure the constraints we assumed (Test function should not affect other test functions). Further, specifying the name of test function supports asterisk(*), thus if you specify “test_” then every function whose name starts with “test_” will be selected. If “tests” is not specified in the configuration file, then FuzzBuilder considers unit tests written with google test. Hence, FuzzBuilder starts to find test functions automatically by using a pattern on test function names in google test.
“skips” is for specifying test function names need to be skipped. This is used to prevent low execution speed. For example, if “test_stress” function takes around 10 seconds, it will decrease fuzzing performance of generated executables significantly. Hence, this option can be used to remove “test_stress” function in generated executables.
Please see the 3.1 section in the paper for more detailed explanation.

(3) Configuration examples 

The Configuration file has JSON format. The following are three examples of configuration files.

{

    "targets" : 
    
        [ [ "ares_parse_a_reply", 1, 2], [ "ares_parse_aaaa_reply", 1, 2 ], ["ares_parse_ptr_reply", 1, 2 ],	
	  ["ares_parse_ns_reply", 1, 2], ["ares_parse_srv_reply", 1, 2], ["ares_parse_mx_reply", 1, 2],
	  ["ares_parse_txt_reply", 1, 2], ["ares_parse_soa_reply", 1, 2], ["ares_parse_naptr_reply", 1, 2]],
	  
    "files" :
    
        [ "ares-test-main.bc", "ares-test.bc", "ares-test-internal.bc",
	"dns-proto.bc", "ares-test-parse-a.bc", "ares-test-parse.bc",
	"ares-test-parse-aaaa.bc", "ares-test-parse-ptr.bc", "ares-test-parse-ns.bc",
	"ares-test-parse-srv.bc", "ares-test-parse-mx.bc", "ares-test-parse-txt.bc",
	"ares-test-parse-soa.bc", "ares-test-parse-naptr.bc" ]
	
}

In this example, 9 FA and related 14 bitcode files were specified. Because there is no “tests” in this configuration file, FuzzBuilder will consider those unit test codes are written with google test, and then find test function automatically.

{

    "targets": [ ["XML_Parse", 2, 3] ],
    
    "tests": [ "test_" ],
    
    "files": [ "runtests.bc" ]
    
}
 
In this example, 1 FA and 1 bit code file were specified. Further, “tests” was specified as “test_”. By this configuration, FuzzBuilder considers every function whose name starts with “test_” as test functions.
 
{

    "targets" : [ ["XML_Parse",2, 3]],
    
    "files" : ["xmlparse.bc" ]    
}  

This example is for generating seeds. XML_Parse function will be instrumented to dump the input passed as the 2nd and the 3rd parameters in xmlparse.bc file.

4) Seed Generation

FuzzBuilder stores every seed that is generated by instrumented unit test executables into /tmp/fuzzbuilder.collect. Fuzzbuilder always appends seeds into that file. Therefore, it should be deleted before collecting seeds. Moreover, the collected seeds come from various FAs. Because it is recommended to generate executable separately for each FA function, those seeds need to be categorized based on where they come. Further, each seed should be stored as a separate file for being used by a seed file.
 
seed_maker.py that is in ${fuzzbuilder repo}/script helps to generate seed files from a collected seed file. This script divides every seed into separate files, and stores each separate file into the directory whose name is FA that uses a seed. Usage of this script is the following.

python seed_maker.py ${collected seeds} ${output directory}

5) Summary of the overall process

Following are the detailed steps to generate seeds by using FuzzBuilder.

(1) Find source code where FA is defined.

(2) Generate LLVM bitcode files of source code.

(3) Write configuration file to instrument source code.

(4) Run FuzzBuilder with seed option and written configuration file.

(5) Build library and unit test with the instrumented source codes.

(6) Remove /tmp/fuzzbuilder.collect file before executing a unit test.

(7) Run a unit test.

(8) Run : python ${fuzzbuilder repo}/script/seed_maker.py /tmp/fuzzbuilder.collect ${output}


Following are the detailed steps to generate executables by using FuzzBuilder.

(1) Find unit test source code where FA is tested.

(2) Generate LLVM bitcode of unit test source codes.

(3) Write configuration file to instrument source code.

(4) Run FuzzBuilder with exec option and written configuration file.

(5) Build unit test with instrumented unit test source codes.

(6) Start fuzzing with the built unit test through stdin interface. (Currently, the only stdin is supported.)

6) Detailed example of appliance of FuzzBuilder

In this example, we will attempt to apply FuzzBuilder to expat project. 

(1) Download source code of expat project

mkdir -p ${fuzzbuilder repo}/exp/expat/source

cd ${fuzzbuilder repo}/exp/expat/source

git clone https://github.com/libexpat/libexpat

cd ${fuzzbuilder repo}/exp/expat/source/libexpat

git checkout 39e487da353b20bb3a724311d179ba0fddffc65b

cp ${fuzzbuilder repo}/exp/expat/fuzzer_main.cc ${fuzzbuilder repo}/exp/expat/source/libexpat/expat

Now, your build environment for expat project was prepared in ${fuzzbuilder repo}/exp/expat/source/libexpat/expat.
 
(2) Build the project

cd ${fuzzbuilder repo}/exp/expat/source/libexpat/expat

./buildconf.sh

./configure CC=afl-clang CXX=afl-clang++ CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32

make

make check
 
Now, you have libexpat library(lib/.libs/libexpat.a) and its unit test executable(tests/runtests)
 
(3) Applying FuzzBuilder to generated seeds

afl-clang -emit-llvm -DHAVE_CONFIG_H -I. -I.. -DHAVE_EXPAT_CONFIG_H -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -c lib/xmlparse.c -fPIC -DPIC

${fuzzbuilder repo}/src/build/fuzzbuilder seed ${fuzzbuilder repo}/exp/expat/seed.conf
 
Now, you have instrumented bitcode file whose name is xmlparse.bc.mod.bc
 
(4) Build a unit test with instrumented bitcode file

Before start, this step can be performed differently. It is because automation of build process is not considered in FuzzBuilder. In the future, we are trying to automate this process to minimize human interaction.

afl-clang -DHAVE_CONFIG_H -I. -I.. -DHAVE_EXPAT_CONFIG_H -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -c xmlparse.bc.mod.bc -fPIC -DPIC -o xmlparse.o

ar r lib/.libs/libexpat.a xmlparse.o

rm -f tests/runtests

make check
 
(5) Run a unit test to collect seed files.

rm -f /tmp/fuzzbuilder.collect

make check

mv /tmp/fuzzbuilder.collect .

python ${fuzzbuilder repo}/tool/seed_maker.py fuzzbuilder.collect seed_fb

mkdir -p ${fuzzbuilder repo}/exp/expat/seed/fuzzbuilder

mv ${fuzzbuilder repo}/exp/expat/source/libexpat/expat/seed_fb ${fuzzbuilder repo}/exp/expat/seed/fuzzbuilder/org
 
Now, you have all seed files in ${fuzzbuilder repo}/exp/expat/seed/fuzzbuilder/org/XML_Parse.
Optionally, you can minimize seed files by using afl-cmin that is built along with afl-fuzz.
 
(6) FuzzBuilder to generate executables.

cd {fuzzbuilder repo}/exp/expat/source/libexpat/expat/tests

afl-clang -emit-llvm -DHAVE_CONFIG_H -I. -I.. -I./../lib -DHAVE_EXPAT_CONFIG_H -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -c runtests.c

${fuzzbuilder repo}/src/build/fuzzbuilder exec ${fuzzbuilder repo}/exp/expat/XML_Parse.conf

afl-clang -DHAVE_CONFIG_H -I. -I.. -I./../lib -DHAVE_EXPAT_CONFIG_H -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -o runtests.o -c runtests.bc.mod.bc

AFL_USE_ASAN=1 afl-clang -m32 -g -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -fno-strict-aliasing -m32 –g -fno-strict-aliasing -o XML_Parse_fuzzer runtests.o libruntests.a ../lib/.libs/libexpat.a

mkdir -p {fuzzbuilder repo}/exp/expat/bin/fuzz/fuzzbuilder

mv XML_Parse_fuzzer ${fuzzbuilder repo}/exp/expat/bin/fuzz/fuzzbuilder

Now, generated fuzz-able executable is stored in {fuzzbuilder repo}/exp/expat/bin/fuzz/fuzzbuilder/XML_Parse_fuzzer

7) Building docker image

Build a docker image for setting up the evaluation environment. There is “Dockerfile” in ${fuzzbuilder repo}. Docker image that has every resource for evaluation can be generated by using docker build. Please try following commands to build docker image from Dockerfile.

cd ${fuzzbuilder repo} && docker build -t fuzzbuilder.eval .

Note that, it takes a lot of time to build docker image. (Most time is for building a library, and corpus minimization by afl-cmin.)

8) Information of Docker image

Each environment of 6 projects (c-ares, expat, boringssl, yara, cjson, mpc) are in the /exp directory. For example, c-ares is in /exp/c-ares. In the directory, you can find 3 sub-directories: bin, seed, source.

"bin" directory stores fuzz-able executables by oss-fuzz and FuzzBuilder. Each of the directories has 2 sub-directories: fuzz, cov. The binaries in "fuzz" directory is built with address sanitizer. The binaries in "cov" directory are built with coverage related information. This information is strongly linked to .gcno files in "sources" directory.

"seed" directory stores seed files by oss-fuzz and FuzzBuilder. You can find 3 sub-directories: fuzzbuilder, oss-fuzz, eval. fuzzbuilder directory also has 2 sub-directories: opt, org. Two directories also have sub-directories. Each of them has related fuzz-able executable name.

”org" stores seed files that are generated by FuzzBuilder. "opt" stores seed files that are minimized by afl-cmin. In the case of oss-fuzz directory, there is the only org directory that has the same purpose as fuzzbuilder.

“eval" directory stores valid seed files that come from either fuzzbuilder or oss-fuzz for related fuzz-able executables. You can find each seed file by following directory structure.

/exp/${package}/seed/eval/${related fuzz-able executable}/oss-fuzz or fuzzbuilder.

Note that, these seed files are for counting valid seed files that is used in Figure 6 in the paper. You can easily calculate the number of seed files by using “wc -l $path” command.

"source" directory has source files of the project. Note that there are a lot of coverage related files. Hence, keep this directory if you want to measure coverage correctly.

9) Fuzzing the program inside the docker container

For example, if you want to replay the result of T1 in figure 7. You can easily do it by using the below commands.
 
For fuzzbuilder,

mkdir -p /exp/c-ares/output/fuzzbuilder

timeout 21600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/c-ares/seed/fuzzbuilder/opt/ares_create_query -o /exp/c-ares/output/fuzzbuilder/ares_create_query_fuzzer /exp/c-ares/bin/fuzz/fuzzbuilder/ares_create_query_fuzzer

For oss-fuzz,

mkdir -p /exp/c-ares/output/oss-fuzz

timeout 21600 /tool/afl-2.52b/afl-fuzz -m 1024 -i /exp/c-ares/seed/eval/ares_create_query/oss-fuzz -o /exp/c-ares/output/oss-fuzz/ares_create_query_fuzzer /exp/c-ares/bin/fuzz/oss-fuzz/ares_create_query_fuzzer @@

Then you will get the result of fuzzing in /exp/c-res/output/fuzzbuilder/ares_create_query_fuzzer for fuzzbuilder and /exp/c-ares/output/oss-fuzz/ares_create_query_fuzzer for oss-fuzz after 21600 seconds.

Note that, before using AFL, you may be required to set kernel configuration like the followings.

echo core | sudo tee -a /proc/sys/kernel/core_pattern

cd /sys/devices/system/cpu && echo performance | tee cpu*/cpufreq/scaling_governor

10) Replaying evaluation

(1) How to measure code coverage.

We used line coverage to measure coverage. Firstly, we built libraries with compiler option "-ftest-coverage –fprofile-arcs”. Then, we ran fuzz-able executables. After the execution, .gcda file are generated that can be used by llvm-cov tool. We ran llvm-cov tool for each. gcda files and collected every line coverage of each file. Because test code should not be excluded in the measurement, we generate a simple rule to exclude test code files. These all steps are included in python script, coverage.py that is in /tool directory in docker container.

(2) How to measure called library API functions (Table 9, Table 10)

Firstly, we should prepare list of all library API functions. For this, we used "nm –D ${library}” command to get dynamic symbols in libraries. Then, we collected every symbol that is in text section. This list is stored in each project directory in a docker container, for example /exp/c-ares/apis.txt.

Next steps are almost the same as the above steps to measure code coverage. We used –f option for llvm-cov to get line coverage of each function. By using this tool, we could collect line coverage of every function. Then we remove not visited function from the collected information. Then, we compare visited function with the functions that were stored in apis.txt file. By this way, we can check how many library API functions were visited. These all steps without preparing apis.txt are included in python script, coverage_api.py that is in /tool directory in a docker container.

(3) coverage.py

This script requires mandatory 6 arguments. Below table describes parameters for using this script.

Index	Description
1	A path of workspace. In docker file, workspace will be /exp/c-ares, /exp/expat, /exp/boringssl etc.

2	ID. It is just to identify each result.

3	An interface type for executing fuzz-able executable. "file” means that input is sent via file, "stdin” means that input is sent via stdin. "file” is used for executables from oss-fuzz. "stdin” is used for executables by FuzzBuilder.

4	A path of source directory. An absolute path of source directory is made by concatenating this string to the path of workspace that is specified in the first parameter.

5	Type of a measurement. “seed” is used to see only final coverage after every seed was tested. “exec” is used to see how coverage changes over time by using sorted seeds.

6	A name of output file.

And then, we should specify which executable and seed files should be used to measure coverage. For this, a pair of an executable path and seed file path should be specified.

For example, 

bin/cov/oss-fuzz/ares_create_query_fuzzer seed/eval/ares_create_query/fuzzbuilder

this mans that an executable is bin/cov/oss-fuzz/ares_create_query_fuzzer, and seed files are stored in seed/eval/ares_create_query/fuzzbuilder.

Thus full parameters of coverage.py will be the following.

python /tool/coverage.py /exp/c-ares c1 file source/c-ares seed seed_c1_f.txt bin/cov/oss-fuzz/ares_create_query_fuzzer seed/eval/ares_create_query/fuzzbuilder

(4) Actual steps

We will show actual steps to do it, if we want to show the result of T1 in figure 7.
 
For fc1,

python /tool/coverage.py /exp/c-ares fc1 stdin source/c-ares exec exec_fc1_f.txt bin/cov/fuzzbuilder/ares_create_query_fuzzer output/fuzzbuilder/ares_create_query_fuzzer/queue

For c1,

python /tool/coverage.py /exp/c-ares c1 file source/c-ares exec exec_c1_o.txt bin/cov/oss-fuzz/ares_create_query_fuzzer output/oss-fuzz/ares_create_query_fuzzer/queue
 
This script assumes that the result of AFL for fc1 is stored in /exp/c-ares/output/fuzzbuilder/ares_create_query_fuzzer. In case of the c1, /exp/c-ares/output/oss-fuzz/ares_create_query_fuzzer. You can see how line coverage was changed over time by reading exec_fc1_f.txt and exec_c1_o.txt file respectively.

(5) Measuring the number of valid seed files.

For example, if you want to replay the result of c1 in Figure 6 and c1 in Table 6. You can easily do it by using the following commands.

For fuzzbuilder,
python /tool/coverage.py /exp/c-ares c1 file source/c-ares seed seed_c1_f.txt bin/cov/oss-fuzz/ares_create_query_fuzzer seed/eval/ares_create_query/fuzzbuilder

For oss-fuzz,
python /tool/coverage.py /exp/c-ares c1 file source/c-ares seed seed_c1_o.txt bin/cov/oss-fuzz/ares_create_query_fuzzer seed/eval/ares_create_query/oss-fuzz

Then, you can check the result by reading seed_c1_f.txt and seed_c1_o.txt respectively.

(6) Measuring the number of called library API functions.

If you want to replay c-ares in Table 9, you can do it by using below commands.
 
For fuzzbuilder,

python /tool/coverage_api.py /exp/c-ares/apis.txt /exp/c-ares source/c-ares api_cares_f.txt fuzz bin/cov/fuzzbuilder/ares_create_query_fuzzer output/fuzzbuilder/ares_create_query_fuzzer/queue bin/cov/fuzzbuilder/ares_parse_reply_fuzzer output/fuzzbuilder/ares_parse_reply_fuzzer/queue

For unit tests,

python /tool/coverage_api.py /exp/c-ares/apis.txt /exp/c-ares source/c-ares api_cares_u.txt unit source/c-ares/test/arestest

Every command for replaying the evaluation is supported in exp/eval.txt in the github repository. 

11) Replaying bugs

We prepared fuzz-able executables for replaying bugs in expat, cJSON and mpc. The fuzz-able executables wer generated with a specific configuration file while docker is building. We hope that you can reproduce bugs that we found by fuzzing them. Below is command sets to fuzz those executables.

For expat

/tool/afl-2.52b/afl-fuzz -i /exp/expat/seed/fuzzbuilder/opt/XML_Parse -o /exp/expat/expat_bug -m 1024 /exp/expat/bin/fuzz/fuzzbuilder/bug_fuzzer

For cJSON

/tool/afl-2.52b/afl-fuzz -i /exp/cJSON/seed/fuzzbuilder/org/cJSON_ParseWithOpts -o /exp/expat/cJSON_bug -m 1024 /exp/cJSON/bin/fuzz/fuzzbuilder/misc_tests

For mpc

/tool/afl-2.52b/afl-fuzz -i /exp/mpc/seed/fuzzbuilder/opt/mpc_test_pass -o /exp/mpc/mpc_bug_1 -m 1024 /exp/mpc/bin/fuzz/fuzzbuilder/mpc_test_pass

/tool/afl-2.52b/afl-fuzz -i /exp/mpc/seed/fuzzbuilder/opt/mpca_lang/ -o /exp/mpc/mpc_bug_2 -m 1024 /exp/mpc/bin/fuzz/fuzzbuilder/mpca_lang
