# Split fuzz driver
If you want to split a fuzz driver into several executable files, one per FA, you MUST change the branch from master to develop.

## Example for 'c-ares' project

### 1. Download source code of c-ares project
```bash
mkdir -p $FuzzBuilder/exp/c-ares/source
cd $FuzzBuilder/exp/c-ares/source
git clone https://github.com/c-ares/c-ares.git
cd $FuzzBuilder/exp/c-ares/source/c-ares
git checkout a9c2068e25a107bf535b1fc988eec47384b86dc6
cp $FuzzBuilder/exp/c-ares/build.sh $FuzzBuilder/exp/c-ares/source/c-ares
cp $FuzzBuilder/exp/c-ares/fuzzer_main.cc $FuzzBuilder/exp/c-ares/source/c-ares
```

### 2. Build the project
```bash
cd $FuzzBuilder/exp/c-ares/source/c-ares
rm -f $(find . -name "*.bc")
# edit CC, CXX variable with correct path
vi ./build.sh
./build.sh seed
```

### 3. Applying FuzzBuilder to generate seeds
```bash
afl-clang -emit-llvm -DHAVE_CONFIG_H -I. -I. -DCARES_BUILDING_LIBRARY -DCARES_SYMBOL_HIDING -fvisibility=hidden -m32 -Qunused-arguments -g0 -Os -fPIC -DPIC -c ares_create_query.c ares_parse_a_reply.c ares_parse_aaaa_reply.c ares_parse_mx_reply.c ares_parse_naptr_reply.c ares_parse_ns_reply.c ares_parse_ptr_reply.c ares_parse_soa_reply.c ares_parse_srv_reply.c ares_parse_txt_reply.c
$FuzzBuilder/build/fuzzbuilder seed $FuzzBuilder/exp/c-ares/seed.conf
```

### 4. Build a unit test with instrumented bitcode file
```bash
afl-clang -DHAVE_CONFIG_H -I. -I. -DCARES_BUILDING_LIBRARY -DCARES_SYMBOL_HIDING -fvisibility=hidden -m32 -Qunused-arguments -g0 -Os -fPIC -DPIC -c ares_create_query.bc.mod.bc ares_parse_a_reply.bc.mod.bc ares_parse_aaaa_reply.bc.mod.bc ares_parse_mx_reply.bc.mod.bc ares_parse_naptr_reply.bc.mod.bc ares_parse_ns_reply.bc.mod.bc ares_parse_ptr_reply.bc.mod.bc ares_parse_soa_reply.bc.mod.bc ares_parse_srv_reply.bc.mod.bc ares_parse_txt_reply.bc.mod.bc
mv ares_create_query.bc.mod.o libcares_la-ares_create_query.o
mv ares_parse_a_reply.bc.mod.o libcares_la-ares_parse_a_reply.o
mv ares_parse_aaaa_reply.bc.mod.o libcares_la-ares_parse_aaaa_reply.o
mv ares_parse_mx_reply.bc.mod.o libcares_la-ares_parse_mx_reply.o
mv ares_parse_naptr_reply.bc.mod.o libcares_la-ares_parse_naptr_reply.o
mv ares_parse_ns_reply.bc.mod.o libcares_la-ares_parse_ns_reply.o
mv ares_parse_ptr_reply.bc.mod.o libcares_la-ares_parse_ptr_reply.o
mv ares_parse_soa_reply.bc.mod.o libcares_la-ares_parse_soa_reply.o
mv ares_parse_srv_reply.bc.mod.o libcares_la-ares_parse_srv_reply.o
mv ares_parse_txt_reply.bc.mod.o libcares_la-ares_parse_txt_reply.o
ar r .libs/libcares.a libcares_la-ares_create_query.o libcares_la-ares_parse*.o
rm -rf /tmp/fuzzbuilder.collect
cd $FuzzBuilder/exp/c-ares/source/c-ares/test
rm -rf arestest
make && (./arestest || set ?=0)
```

### 5. Run a unit test to collect seed files
```bash
mv /tmp/fuzzbuilder.collect .
python $FuzzBuilder/script/seed_maker.py fuzzbuilder.collect seed_fb
mkdir -p $FuzzBuilder/exp/c-ares/seed/fuzzbuilder/org
mv $FuzzBuilder/exp/c-ares/source/c-ares/test/seed_fb/* $FuzzBuilder/exp/c-ares/seed/fuzzbuilder/org
```

### 6. FuzzBuilder to generate fuzzing executables
```bash
cd $FuzzBuilder/exp/c-ares/source/c-ares/test
afl-clang++ -emit-llvm -DHAVE_CONFIG_H -I. -I.. -isystem gmock-1.8.0 -m32 -g -Wall -pthread -c ares-test-main.cc ares-test-misc.cc ares-test.cc ares-test-internal.cc dns-proto.cc ares-test-parse-a.cc ares-test-parse.cc ares-test-parse-aaaa.cc ares-test-parse-ptr.cc ares-test-parse-ns.cc ares-test-parse-srv.cc ares-test-parse-txt.cc ares-test-parse-soa.cc ares-test-parse-naptr.cc ares-test-parse-naptr.cc ares-test-parse-mx.cc
$FuzzBuilder/build/fuzzbuilder exec $FuzzBuilder/exp/c-ares/ares_create_query.conf
afl-clang++ -DHAVE_CONFIG_H -I. -I.. -isystem gmock-1.8.0 -m32 -g -Wall -pthread -c ares-test-main.bc.mod.bc ares-test-misc.bc.mod.bc ares-test.bc.mod.bc ares-test-internal.bc.mod.bc dns-proto.bc.mod.bc
afl-clang++ -m32 -Wall -pthread -o ares_create_query_fuzzer ares-test-main.bc.mod.o ares-test-misc.bc.mod.o ares-test.bc.mod.o ares-test-internal.bc.mod.o dns-proto.bc.mod.o .libs/libgmock.a ../.libs/libcares.a

$FuzzBuilder/build/fuzzbuilder exec $FuzzBuilder/exp/c-ares/ares_parse_reply.conf
afl-clang++ -DHAVE_CONFIG_H -I. -I.. -isystem gmock-1.8.0 -m32 -g -Wall -pthread -c ares-test-main.bc.mod_ares_parse_aaaa_reply.bc ares-test.bc.mod_ares_parse_aaaa_reply.bc ares-test-internal.bc.mod_ares_parse_aaaa_reply.bc dns-proto.bc.mod_ares_parse_aaaa_reply.bc ares-test-parse-aaaa.bc.mod_ares_parse_aaaa_reply.bc ares-test-parse-a.bc.mod_ares_parse_aaaa_reply.bc ares-test-parse.bc.mod_ares_parse_aaaa_reply.bc ares-test-parse-mx.bc.mod_ares_parse_aaaa_reply.bc ares-test-parse-naptr.bc.mod_ares_parse_aaaa_reply.bc ares-test-parse-ns.bc.mod_ares_parse_aaaa_reply.bc ares-test-parse-ptr.bc.mod_ares_parse_aaaa_reply.bc ares-test-parse-soa.bc.mod_ares_parse_aaaa_reply.bc ares-test-parse-srv.bc.mod_ares_parse_aaaa_reply.bc ares-test-parse-txt.bc.mod_ares_parse_aaaa_reply.bc
afl-clang++ -m32 -g -Wall -pthread -o ares_parse_aaaa_reply_fuzzer ares-test-main.bc.mod_ares_parse_aaaa_reply.o ares-test.bc.mod_ares_parse_aaaa_reply.o ares-test-internal.bc.mod_ares_parse_aaaa_reply.o dns-proto.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-a.bc.mod_ares_parse_aaaa_reply.o ares-test-parse.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-mx.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-naptr.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-ns.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-ptr.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-soa.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-srv.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-txt.bc.mod_ares_parse_aaaa_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -DHAVE_CONFIG_H -I. -I.. -isystem gmock-1.8.0 -m32 -g -Wall -pthread -c ares-test-main.bc.mod_ares_parse_a_reply.bc ares-test.bc.mod_ares_parse_a_reply.bc ares-test-internal.bc.mod_ares_parse_a_reply.bc dns-proto.bc.mod_ares_parse_a_reply.bc ares-test-parse-aaaa.bc.mod_ares_parse_a_reply.bc ares-test-parse-a.bc.mod_ares_parse_a_reply.bc ares-test-parse.bc.mod_ares_parse_a_reply.bc ares-test-parse-mx.bc.mod_ares_parse_a_reply.bc ares-test-parse-naptr.bc.mod_ares_parse_a_reply.bc ares-test-parse-ns.bc.mod_ares_parse_a_reply.bc ares-test-parse-ptr.bc.mod_ares_parse_a_reply.bc ares-test-parse-soa.bc.mod_ares_parse_a_reply.bc ares-test-parse-srv.bc.mod_ares_parse_a_reply.bc ares-test-parse-txt.bc.mod_ares_parse_a_reply.bc
afl-clang++ -m32 -g -Wall -pthread -o ares_parse_a_reply_fuzzer ares-test-main.bc.mod_ares_parse_a_reply.o ares-test.bc.mod_ares_parse_a_reply.o ares-test-internal.bc.mod_ares_parse_a_reply.o dns-proto.bc.mod_ares_parse_a_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_a_reply.o ares-test-parse-a.bc.mod_ares_parse_a_reply.o ares-test-parse.bc.mod_ares_parse_a_reply.o ares-test-parse-mx.bc.mod_ares_parse_a_reply.o ares-test-parse-naptr.bc.mod_ares_parse_a_reply.o ares-test-parse-ns.bc.mod_ares_parse_a_reply.o ares-test-parse-ptr.bc.mod_ares_parse_a_reply.o ares-test-parse-soa.bc.mod_ares_parse_a_reply.o ares-test-parse-srv.bc.mod_ares_parse_a_reply.o ares-test-parse-txt.bc.mod_ares_parse_a_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -DHAVE_CONFIG_H -I. -I.. -isystem gmock-1.8.0 -m32 -g -Wall -pthread -c ares-test-main.bc.mod_ares_parse_ptr_reply.bc ares-test.bc.mod_ares_parse_ptr_reply.bc ares-test-internal.bc.mod_ares_parse_ptr_reply.bc dns-proto.bc.mod_ares_parse_ptr_reply.bc ares-test-parse-aaaa.bc.mod_ares_parse_ptr_reply.bc ares-test-parse-a.bc.mod_ares_parse_ptr_reply.bc ares-test-parse.bc.mod_ares_parse_ptr_reply.bc ares-test-parse-mx.bc.mod_ares_parse_ptr_reply.bc ares-test-parse-naptr.bc.mod_ares_parse_ptr_reply.bc ares-test-parse-ns.bc.mod_ares_parse_ptr_reply.bc ares-test-parse-ptr.bc.mod_ares_parse_ptr_reply.bc ares-test-parse-soa.bc.mod_ares_parse_ptr_reply.bc ares-test-parse-srv.bc.mod_ares_parse_ptr_reply.bc ares-test-parse-txt.bc.mod_ares_parse_ptr_reply.bc
afl-clang++ -m32 -g -Wall -pthread -o ares_parse_ptr_reply_fuzzer ares-test-main.bc.mod_ares_parse_ptr_reply.o ares-test.bc.mod_ares_parse_ptr_reply.o ares-test-internal.bc.mod_ares_parse_ptr_reply.o dns-proto.bc.mod_ares_parse_ptr_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_ptr_reply.o ares-test-parse-a.bc.mod_ares_parse_ptr_reply.o ares-test-parse.bc.mod_ares_parse_ptr_reply.o ares-test-parse-mx.bc.mod_ares_parse_ptr_reply.o ares-test-parse-naptr.bc.mod_ares_parse_ptr_reply.o ares-test-parse-ns.bc.mod_ares_parse_ptr_reply.o ares-test-parse-ptr.bc.mod_ares_parse_ptr_reply.o ares-test-parse-soa.bc.mod_ares_parse_ptr_reply.o ares-test-parse-srv.bc.mod_ares_parse_ptr_reply.o ares-test-parse-txt.bc.mod_ares_parse_ptr_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -DHAVE_CONFIG_H -I. -I.. -isystem gmock-1.8.0 -m32 -g -Wall -pthread -c ares-test-main.bc.mod_ares_parse_ns_reply.bc ares-test.bc.mod_ares_parse_ns_reply.bc ares-test-internal.bc.mod_ares_parse_ns_reply.bc dns-proto.bc.mod_ares_parse_ns_reply.bc ares-test-parse-aaaa.bc.mod_ares_parse_ns_reply.bc ares-test-parse-a.bc.mod_ares_parse_ns_reply.bc ares-test-parse.bc.mod_ares_parse_ns_reply.bc ares-test-parse-mx.bc.mod_ares_parse_ns_reply.bc ares-test-parse-naptr.bc.mod_ares_parse_ns_reply.bc ares-test-parse-ns.bc.mod_ares_parse_ns_reply.bc ares-test-parse-ptr.bc.mod_ares_parse_ns_reply.bc ares-test-parse-soa.bc.mod_ares_parse_ns_reply.bc ares-test-parse-srv.bc.mod_ares_parse_ns_reply.bc ares-test-parse-txt.bc.mod_ares_parse_ns_reply.bc
afl-clang++ -m32 -g -Wall -pthread -o ares_parse_ns_reply_fuzzer ares-test-main.bc.mod_ares_parse_ns_reply.o ares-test.bc.mod_ares_parse_ns_reply.o ares-test-internal.bc.mod_ares_parse_ns_reply.o dns-proto.bc.mod_ares_parse_ns_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_ns_reply.o ares-test-parse-a.bc.mod_ares_parse_ns_reply.o ares-test-parse.bc.mod_ares_parse_ns_reply.o ares-test-parse-mx.bc.mod_ares_parse_ns_reply.o ares-test-parse-naptr.bc.mod_ares_parse_ns_reply.o ares-test-parse-ns.bc.mod_ares_parse_ns_reply.o ares-test-parse-ptr.bc.mod_ares_parse_ns_reply.o ares-test-parse-soa.bc.mod_ares_parse_ns_reply.o ares-test-parse-srv.bc.mod_ares_parse_ns_reply.o ares-test-parse-txt.bc.mod_ares_parse_ns_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -DHAVE_CONFIG_H -I. -I.. -isystem gmock-1.8.0 -m32 -g -Wall -pthread -c ares-test-main.bc.mod_ares_parse_srv_reply.bc ares-test.bc.mod_ares_parse_srv_reply.bc ares-test-internal.bc.mod_ares_parse_srv_reply.bc dns-proto.bc.mod_ares_parse_srv_reply.bc ares-test-parse-aaaa.bc.mod_ares_parse_srv_reply.bc ares-test-parse-a.bc.mod_ares_parse_srv_reply.bc ares-test-parse.bc.mod_ares_parse_srv_reply.bc ares-test-parse-mx.bc.mod_ares_parse_srv_reply.bc ares-test-parse-naptr.bc.mod_ares_parse_srv_reply.bc ares-test-parse-ns.bc.mod_ares_parse_srv_reply.bc ares-test-parse-ptr.bc.mod_ares_parse_srv_reply.bc ares-test-parse-soa.bc.mod_ares_parse_srv_reply.bc ares-test-parse-srv.bc.mod_ares_parse_srv_reply.bc ares-test-parse-txt.bc.mod_ares_parse_srv_reply.bc
afl-clang++ -m32 -g -Wall -pthread -o ares_parse_srv_reply_fuzzer ares-test-main.bc.mod_ares_parse_srv_reply.o ares-test.bc.mod_ares_parse_srv_reply.o ares-test-internal.bc.mod_ares_parse_srv_reply.o dns-proto.bc.mod_ares_parse_srv_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_srv_reply.o ares-test-parse-a.bc.mod_ares_parse_srv_reply.o ares-test-parse.bc.mod_ares_parse_srv_reply.o ares-test-parse-mx.bc.mod_ares_parse_srv_reply.o ares-test-parse-naptr.bc.mod_ares_parse_srv_reply.o ares-test-parse-ns.bc.mod_ares_parse_srv_reply.o ares-test-parse-ptr.bc.mod_ares_parse_srv_reply.o ares-test-parse-soa.bc.mod_ares_parse_srv_reply.o ares-test-parse-srv.bc.mod_ares_parse_srv_reply.o ares-test-parse-txt.bc.mod_ares_parse_srv_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -DHAVE_CONFIG_H -I. -I.. -isystem gmock-1.8.0 -m32 -g -Wall -pthread -c ares-test-main.bc.mod_ares_parse_mx_reply.bc ares-test.bc.mod_ares_parse_mx_reply.bc ares-test-internal.bc.mod_ares_parse_mx_reply.bc dns-proto.bc.mod_ares_parse_mx_reply.bc ares-test-parse-aaaa.bc.mod_ares_parse_mx_reply.bc ares-test-parse-a.bc.mod_ares_parse_mx_reply.bc ares-test-parse.bc.mod_ares_parse_mx_reply.bc ares-test-parse-mx.bc.mod_ares_parse_mx_reply.bc ares-test-parse-naptr.bc.mod_ares_parse_mx_reply.bc ares-test-parse-ns.bc.mod_ares_parse_mx_reply.bc ares-test-parse-ptr.bc.mod_ares_parse_mx_reply.bc ares-test-parse-soa.bc.mod_ares_parse_mx_reply.bc ares-test-parse-srv.bc.mod_ares_parse_mx_reply.bc ares-test-parse-txt.bc.mod_ares_parse_mx_reply.bc
afl-clang++ -m32 -g -Wall -pthread -o ares_parse_mx_reply_fuzzer ares-test-main.bc.mod_ares_parse_mx_reply.o ares-test.bc.mod_ares_parse_mx_reply.o ares-test-internal.bc.mod_ares_parse_mx_reply.o dns-proto.bc.mod_ares_parse_mx_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_mx_reply.o ares-test-parse-a.bc.mod_ares_parse_mx_reply.o ares-test-parse.bc.mod_ares_parse_mx_reply.o ares-test-parse-mx.bc.mod_ares_parse_mx_reply.o ares-test-parse-naptr.bc.mod_ares_parse_mx_reply.o ares-test-parse-ns.bc.mod_ares_parse_mx_reply.o ares-test-parse-ptr.bc.mod_ares_parse_mx_reply.o ares-test-parse-soa.bc.mod_ares_parse_mx_reply.o ares-test-parse-srv.bc.mod_ares_parse_mx_reply.o ares-test-parse-txt.bc.mod_ares_parse_mx_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -DHAVE_CONFIG_H -I. -I.. -isystem gmock-1.8.0 -m32 -g -Wall -pthread -c ares-test-main.bc.mod_ares_parse_txt_reply.bc ares-test.bc.mod_ares_parse_txt_reply.bc ares-test-internal.bc.mod_ares_parse_txt_reply.bc dns-proto.bc.mod_ares_parse_txt_reply.bc ares-test-parse-aaaa.bc.mod_ares_parse_txt_reply.bc ares-test-parse-a.bc.mod_ares_parse_txt_reply.bc ares-test-parse.bc.mod_ares_parse_txt_reply.bc ares-test-parse-mx.bc.mod_ares_parse_txt_reply.bc ares-test-parse-naptr.bc.mod_ares_parse_txt_reply.bc ares-test-parse-ns.bc.mod_ares_parse_txt_reply.bc ares-test-parse-ptr.bc.mod_ares_parse_txt_reply.bc ares-test-parse-soa.bc.mod_ares_parse_txt_reply.bc ares-test-parse-srv.bc.mod_ares_parse_txt_reply.bc ares-test-parse-txt.bc.mod_ares_parse_txt_reply.bc
afl-clang++ -m32 -g -Wall -pthread -o ares_parse_txt_reply_fuzzer ares-test-main.bc.mod_ares_parse_txt_reply.o ares-test.bc.mod_ares_parse_txt_reply.o ares-test-internal.bc.mod_ares_parse_txt_reply.o dns-proto.bc.mod_ares_parse_txt_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_txt_reply.o ares-test-parse-a.bc.mod_ares_parse_txt_reply.o ares-test-parse.bc.mod_ares_parse_txt_reply.o ares-test-parse-mx.bc.mod_ares_parse_txt_reply.o ares-test-parse-naptr.bc.mod_ares_parse_txt_reply.o ares-test-parse-ns.bc.mod_ares_parse_txt_reply.o ares-test-parse-ptr.bc.mod_ares_parse_txt_reply.o ares-test-parse-soa.bc.mod_ares_parse_txt_reply.o ares-test-parse-srv.bc.mod_ares_parse_txt_reply.o ares-test-parse-txt.bc.mod_ares_parse_txt_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -DHAVE_CONFIG_H -I. -I.. -isystem gmock-1.8.0 -m32 -g -Wall -pthread -c ares-test-main.bc.mod_ares_parse_soa_reply.bc ares-test.bc.mod_ares_parse_soa_reply.bc ares-test-internal.bc.mod_ares_parse_soa_reply.bc dns-proto.bc.mod_ares_parse_soa_reply.bc ares-test-parse-aaaa.bc.mod_ares_parse_soa_reply.bc ares-test-parse-a.bc.mod_ares_parse_soa_reply.bc ares-test-parse.bc.mod_ares_parse_soa_reply.bc ares-test-parse-mx.bc.mod_ares_parse_soa_reply.bc ares-test-parse-naptr.bc.mod_ares_parse_soa_reply.bc ares-test-parse-ns.bc.mod_ares_parse_soa_reply.bc ares-test-parse-ptr.bc.mod_ares_parse_soa_reply.bc ares-test-parse-soa.bc.mod_ares_parse_soa_reply.bc ares-test-parse-srv.bc.mod_ares_parse_soa_reply.bc ares-test-parse-txt.bc.mod_ares_parse_soa_reply.bc
afl-clang++ -m32 -g -Wall -pthread -o ares_parse_soa_reply_fuzzer ares-test-main.bc.mod_ares_parse_soa_reply.o ares-test.bc.mod_ares_parse_soa_reply.o ares-test-internal.bc.mod_ares_parse_soa_reply.o dns-proto.bc.mod_ares_parse_soa_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_soa_reply.o ares-test-parse-a.bc.mod_ares_parse_soa_reply.o ares-test-parse.bc.mod_ares_parse_soa_reply.o ares-test-parse-mx.bc.mod_ares_parse_soa_reply.o ares-test-parse-naptr.bc.mod_ares_parse_soa_reply.o ares-test-parse-ns.bc.mod_ares_parse_soa_reply.o ares-test-parse-ptr.bc.mod_ares_parse_soa_reply.o ares-test-parse-soa.bc.mod_ares_parse_soa_reply.o ares-test-parse-srv.bc.mod_ares_parse_soa_reply.o ares-test-parse-txt.bc.mod_ares_parse_soa_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -DHAVE_CONFIG_H -I. -I.. -isystem gmock-1.8.0 -m32 -g -Wall -pthread -c ares-test-main.bc.mod_ares_parse_naptr_reply.bc ares-test.bc.mod_ares_parse_naptr_reply.bc ares-test-internal.bc.mod_ares_parse_naptr_reply.bc dns-proto.bc.mod_ares_parse_naptr_reply.bc ares-test-parse-aaaa.bc.mod_ares_parse_naptr_reply.bc ares-test-parse-a.bc.mod_ares_parse_naptr_reply.bc ares-test-parse.bc.mod_ares_parse_naptr_reply.bc ares-test-parse-mx.bc.mod_ares_parse_naptr_reply.bc ares-test-parse-naptr.bc.mod_ares_parse_naptr_reply.bc ares-test-parse-ns.bc.mod_ares_parse_naptr_reply.bc ares-test-parse-ptr.bc.mod_ares_parse_naptr_reply.bc ares-test-parse-soa.bc.mod_ares_parse_naptr_reply.bc ares-test-parse-srv.bc.mod_ares_parse_naptr_reply.bc ares-test-parse-txt.bc.mod_ares_parse_naptr_reply.bc
afl-clang++ -m32 -g -Wall -pthread -o ares_parse_naptr_reply_fuzzer ares-test-main.bc.mod_ares_parse_naptr_reply.o ares-test.bc.mod_ares_parse_naptr_reply.o ares-test-internal.bc.mod_ares_parse_naptr_reply.o dns-proto.bc.mod_ares_parse_naptr_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_naptr_reply.o ares-test-parse-a.bc.mod_ares_parse_naptr_reply.o ares-test-parse.bc.mod_ares_parse_naptr_reply.o ares-test-parse-mx.bc.mod_ares_parse_naptr_reply.o ares-test-parse-naptr.bc.mod_ares_parse_naptr_reply.o ares-test-parse-ns.bc.mod_ares_parse_naptr_reply.o ares-test-parse-ptr.bc.mod_ares_parse_naptr_reply.o ares-test-parse-soa.bc.mod_ares_parse_naptr_reply.o ares-test-parse-srv.bc.mod_ares_parse_naptr_reply.o ares-test-parse-txt.bc.mod_ares_parse_naptr_reply.o .libs/libgmock.a ../.libs/libcares.a

mkdir -p $FuzzBuilder/exp/c-ares/bin/fuzz/fuzzbuilder
mv $FuzzBuilder/exp/c-ares/source/c-ares/test/*_fuzzer $FuzzBuilder/exp/c-ares/bin/fuzz/fuzzbuilder
```

### 7. FuzzBuilder to generate coverage executables
```bash
cd $FuzzBuilder/exp/c-ares/source/c-ares/test

afl-clang++ -fprofile-arcs -ftest-coverage -m32 -Wall -pthread -o ares_create_query_fuzzer ares-test-main.bc.mod.o ares-test-misc.bc.mod.o ares-test.bc.mod.o ares-test-internal.bc.mod.o dns-proto.bc.mod.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -fprofile-arcs -ftest-coverage -m32 -g -Wall -pthread -o ares_parse_aaaa_reply_fuzzer ares-test-main.bc.mod_ares_parse_aaaa_reply.o ares-test.bc.mod_ares_parse_aaaa_reply.o ares-test-internal.bc.mod_ares_parse_aaaa_reply.o dns-proto.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-a.bc.mod_ares_parse_aaaa_reply.o ares-test-parse.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-mx.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-naptr.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-ns.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-ptr.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-soa.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-srv.bc.mod_ares_parse_aaaa_reply.o ares-test-parse-txt.bc.mod_ares_parse_aaaa_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -fprofile-arcs -ftest-coverage -m32 -g -Wall -pthread -o ares_parse_a_reply_fuzzer ares-test-main.bc.mod_ares_parse_a_reply.o ares-test.bc.mod_ares_parse_a_reply.o ares-test-internal.bc.mod_ares_parse_a_reply.o dns-proto.bc.mod_ares_parse_a_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_a_reply.o ares-test-parse-a.bc.mod_ares_parse_a_reply.o ares-test-parse.bc.mod_ares_parse_a_reply.o ares-test-parse-mx.bc.mod_ares_parse_a_reply.o ares-test-parse-naptr.bc.mod_ares_parse_a_reply.o ares-test-parse-ns.bc.mod_ares_parse_a_reply.o ares-test-parse-ptr.bc.mod_ares_parse_a_reply.o ares-test-parse-soa.bc.mod_ares_parse_a_reply.o ares-test-parse-srv.bc.mod_ares_parse_a_reply.o ares-test-parse-txt.bc.mod_ares_parse_a_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -fprofile-arcs -ftest-coverage -m32 -g -Wall -pthread -o ares_parse_ptr_reply_fuzzer ares-test-main.bc.mod_ares_parse_ptr_reply.o ares-test.bc.mod_ares_parse_ptr_reply.o ares-test-internal.bc.mod_ares_parse_ptr_reply.o dns-proto.bc.mod_ares_parse_ptr_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_ptr_reply.o ares-test-parse-a.bc.mod_ares_parse_ptr_reply.o ares-test-parse.bc.mod_ares_parse_ptr_reply.o ares-test-parse-mx.bc.mod_ares_parse_ptr_reply.o ares-test-parse-naptr.bc.mod_ares_parse_ptr_reply.o ares-test-parse-ns.bc.mod_ares_parse_ptr_reply.o ares-test-parse-ptr.bc.mod_ares_parse_ptr_reply.o ares-test-parse-soa.bc.mod_ares_parse_ptr_reply.o ares-test-parse-srv.bc.mod_ares_parse_ptr_reply.o ares-test-parse-txt.bc.mod_ares_parse_ptr_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -fprofile-arcs -ftest-coverage -m32 -g -Wall -pthread -o ares_parse_ns_reply_fuzzer ares-test-main.bc.mod_ares_parse_ns_reply.o ares-test.bc.mod_ares_parse_ns_reply.o ares-test-internal.bc.mod_ares_parse_ns_reply.o dns-proto.bc.mod_ares_parse_ns_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_ns_reply.o ares-test-parse-a.bc.mod_ares_parse_ns_reply.o ares-test-parse.bc.mod_ares_parse_ns_reply.o ares-test-parse-mx.bc.mod_ares_parse_ns_reply.o ares-test-parse-naptr.bc.mod_ares_parse_ns_reply.o ares-test-parse-ns.bc.mod_ares_parse_ns_reply.o ares-test-parse-ptr.bc.mod_ares_parse_ns_reply.o ares-test-parse-soa.bc.mod_ares_parse_ns_reply.o ares-test-parse-srv.bc.mod_ares_parse_ns_reply.o ares-test-parse-txt.bc.mod_ares_parse_ns_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -fprofile-arcs -ftest-coverage -m32 -g -Wall -pthread -o ares_parse_srv_reply_fuzzer ares-test-main.bc.mod_ares_parse_srv_reply.o ares-test.bc.mod_ares_parse_srv_reply.o ares-test-internal.bc.mod_ares_parse_srv_reply.o dns-proto.bc.mod_ares_parse_srv_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_srv_reply.o ares-test-parse-a.bc.mod_ares_parse_srv_reply.o ares-test-parse.bc.mod_ares_parse_srv_reply.o ares-test-parse-mx.bc.mod_ares_parse_srv_reply.o ares-test-parse-naptr.bc.mod_ares_parse_srv_reply.o ares-test-parse-ns.bc.mod_ares_parse_srv_reply.o ares-test-parse-ptr.bc.mod_ares_parse_srv_reply.o ares-test-parse-soa.bc.mod_ares_parse_srv_reply.o ares-test-parse-srv.bc.mod_ares_parse_srv_reply.o ares-test-parse-txt.bc.mod_ares_parse_srv_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -fprofile-arcs -ftest-coverage -m32 -g -Wall -pthread -o ares_parse_mx_reply_fuzzer ares-test-main.bc.mod_ares_parse_mx_reply.o ares-test.bc.mod_ares_parse_mx_reply.o ares-test-internal.bc.mod_ares_parse_mx_reply.o dns-proto.bc.mod_ares_parse_mx_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_mx_reply.o ares-test-parse-a.bc.mod_ares_parse_mx_reply.o ares-test-parse.bc.mod_ares_parse_mx_reply.o ares-test-parse-mx.bc.mod_ares_parse_mx_reply.o ares-test-parse-naptr.bc.mod_ares_parse_mx_reply.o ares-test-parse-ns.bc.mod_ares_parse_mx_reply.o ares-test-parse-ptr.bc.mod_ares_parse_mx_reply.o ares-test-parse-soa.bc.mod_ares_parse_mx_reply.o ares-test-parse-srv.bc.mod_ares_parse_mx_reply.o ares-test-parse-txt.bc.mod_ares_parse_mx_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -fprofile-arcs -ftest-coverage -m32 -g -Wall -pthread -o ares_parse_txt_reply_fuzzer ares-test-main.bc.mod_ares_parse_txt_reply.o ares-test.bc.mod_ares_parse_txt_reply.o ares-test-internal.bc.mod_ares_parse_txt_reply.o dns-proto.bc.mod_ares_parse_txt_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_txt_reply.o ares-test-parse-a.bc.mod_ares_parse_txt_reply.o ares-test-parse.bc.mod_ares_parse_txt_reply.o ares-test-parse-mx.bc.mod_ares_parse_txt_reply.o ares-test-parse-naptr.bc.mod_ares_parse_txt_reply.o ares-test-parse-ns.bc.mod_ares_parse_txt_reply.o ares-test-parse-ptr.bc.mod_ares_parse_txt_reply.o ares-test-parse-soa.bc.mod_ares_parse_txt_reply.o ares-test-parse-srv.bc.mod_ares_parse_txt_reply.o ares-test-parse-txt.bc.mod_ares_parse_txt_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -fprofile-arcs -ftest-coverage -m32 -g -Wall -pthread -o ares_parse_soa_reply_fuzzer ares-test-main.bc.mod_ares_parse_soa_reply.o ares-test.bc.mod_ares_parse_soa_reply.o ares-test-internal.bc.mod_ares_parse_soa_reply.o dns-proto.bc.mod_ares_parse_soa_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_soa_reply.o ares-test-parse-a.bc.mod_ares_parse_soa_reply.o ares-test-parse.bc.mod_ares_parse_soa_reply.o ares-test-parse-mx.bc.mod_ares_parse_soa_reply.o ares-test-parse-naptr.bc.mod_ares_parse_soa_reply.o ares-test-parse-ns.bc.mod_ares_parse_soa_reply.o ares-test-parse-ptr.bc.mod_ares_parse_soa_reply.o ares-test-parse-soa.bc.mod_ares_parse_soa_reply.o ares-test-parse-srv.bc.mod_ares_parse_soa_reply.o ares-test-parse-txt.bc.mod_ares_parse_soa_reply.o .libs/libgmock.a ../.libs/libcares.a

afl-clang++ -fprofile-arcs -ftest-coverage -m32 -g -Wall -pthread -o ares_parse_naptr_reply_fuzzer ares-test-main.bc.mod_ares_parse_naptr_reply.o ares-test.bc.mod_ares_parse_naptr_reply.o ares-test-internal.bc.mod_ares_parse_naptr_reply.o dns-proto.bc.mod_ares_parse_naptr_reply.o ares-test-parse-aaaa.bc.mod_ares_parse_naptr_reply.o ares-test-parse-a.bc.mod_ares_parse_naptr_reply.o ares-test-parse.bc.mod_ares_parse_naptr_reply.o ares-test-parse-mx.bc.mod_ares_parse_naptr_reply.o ares-test-parse-naptr.bc.mod_ares_parse_naptr_reply.o ares-test-parse-ns.bc.mod_ares_parse_naptr_reply.o ares-test-parse-ptr.bc.mod_ares_parse_naptr_reply.o ares-test-parse-soa.bc.mod_ares_parse_naptr_reply.o ares-test-parse-srv.bc.mod_ares_parse_naptr_reply.o ares-test-parse-txt.bc.mod_ares_parse_naptr_reply.o .libs/libgmock.a ../.libs/libcares.a

mkdir -p $FuzzBuilder/exp/c-ares/bin/cov/fuzzbuilder
mv $FuzzBuilder/exp/c-ares/source/c-ares/test/*_fuzzer $FuzzBuilder/exp/c-ares/bin/cov/fuzzbuilder
```

### 8. Seed Optimization (afl-cmin)
```bash
mkdir -p $FuzzBuilder/exp/c-ares/seed/fuzzbuilder/org/ares_parse_reply
mv $FuzzBuilder/exp/c-ares/seed/fuzzbuilder/org/ares_parse_*_reply/* $FuzzBuilder/exp/c-ares/seed/fuzzbuilder/org/ares_parse_reply
rm -rf $FuzzBuilder/exp/c-ares/seed/fuzzbuilder/org/ares_parse_*_reply
mkdir -p $FuzzBuilder/exp/c-ares/seed/fuzzbuilder/opt
cd $FuzzBuilder/exp/c-ares

afl-cmin -m 1024 -i seed/fuzzbuilder/org/ares_create_query -o seed/fuzzbuilder/opt/ares_create_query_fuzzer bin/fuzz/fuzzbuilder/ares_create_query_fuzzer
afl-cmin -m 1024 -i seed/fuzzbuilder/org/ares_parse_reply -o seed/fuzzbuilder/opt/ares_parse_aaaa_reply_fuzzer bin/fuzz/fuzzbuilder/ares_parse_aaaa_reply_fuzzer
afl-cmin -m 1024 -i seed/fuzzbuilder/org/ares_parse_reply -o seed/fuzzbuilder/opt/ares_parse_a_reply_fuzzer bin/fuzz/fuzzbuilder/ares_parse_a_reply_fuzzer
afl-cmin -m 1024 -i seed/fuzzbuilder/org/ares_parse_reply -o seed/fuzzbuilder/opt/ares_parse_ns_reply_fuzzer bin/fuzz/fuzzbuilder/ares_parse_ns_reply_fuzzer
afl-cmin -m 1024 -i seed/fuzzbuilder/org/ares_parse_reply -o seed/fuzzbuilder/opt/ares_parse_ptr_reply_fuzzer bin/fuzz/fuzzbuilder/ares_parse_ptr_reply_fuzzer
afl-cmin -m 1024 -i seed/fuzzbuilder/org/ares_parse_reply -o seed/fuzzbuilder/opt/ares_parse_soa_reply_fuzzer bin/fuzz/fuzzbuilder/ares_parse_soa_reply_fuzzer
afl-cmin -m 1024 -i seed/fuzzbuilder/org/ares_parse_reply -o seed/fuzzbuilder/opt/ares_parse_mx_reply_fuzzer bin/fuzz/fuzzbuilder/ares_parse_mx_reply_fuzzer
afl-cmin -m 1024 -i seed/fuzzbuilder/org/ares_parse_reply -o seed/fuzzbuilder/opt/ares_parse_srv_reply_fuzzer bin/fuzz/fuzzbuilder/ares_parse_srv_reply_fuzzer
afl-cmin -m 1024 -i seed/fuzzbuilder/org/ares_parse_reply -o seed/fuzzbuilder/opt/ares_parse_naptr_reply_fuzzer bin/fuzz/fuzzbuilder/ares_parse_naptr_reply_fuzzer
afl-cmin -m 1024 -i seed/fuzzbuilder/org/ares_parse_reply -o seed/fuzzbuilder/opt/ares_parse_txt_reply_fuzzer bin/fuzz/fuzzbuilder/ares_parse_txt_reply_fuzzer
```

### 9. Fuzzing
```bash
cd $FuzzBuilder
mkdir -p $FuzzBuilder/exp/c-ares/output/fuzzbuilder/ares_create_query_fuzzer
mkdir -p $FuzzBuilder/exp/c-ares/output/fuzzbuilder/ares_parse_aaaa_reply_fuzzer
mkdir -p $FuzzBuilder/exp/c-ares/output/fuzzbuilder/ares_parse_a_reply_fuzzer
mkdir -p $FuzzBuilder/exp/c-ares/output/fuzzbuilder/ares_parse_ptr_reply_fuzzer
mkdir -p $FuzzBuilder/exp/c-ares/output/fuzzbuilder/ares_parse_ns_reply_fuzzer
mkdir -p $FuzzBuilder/exp/c-ares/output/fuzzbuilder/ares_parse_srv_reply_fuzzer
mkdir -p $FuzzBuilder/exp/c-ares/output/fuzzbuilder/ares_parse_mx_reply_fuzzer
mkdir -p $FuzzBuilder/exp/c-ares/output/fuzzbuilder/ares_parse_txt_reply_fuzzer
mkdir -p $FuzzBuilder/exp/c-ares/output/fuzzbuilder/ares_parse_soa_reply_fuzzer
mkdir -p $FuzzBuilder/exp/c-ares/output/fuzzbuilder/ares_parse_naptr_reply_fuzzer

# Optimized seeds (alf-cmin)
afl-fuzz -m 1024 -i exp/c-ares/seed/fuzzbuilder/opt/ares_create_query_fuzzer -o exp/c-ares/output/fuzzbuilder/ares_create_query_fuzzer exp/c-ares/bin/fuzz/fuzzbuilder/ares_create_query_fuzzer
afl-fuzz -m 1024 -i exp/c-ares/seed/fuzzbuilder/opt/ares_parse_aaaa_reply_fuzzer -o exp/c-ares/output/fuzzbuilder/ares_parse_aaaa_reply_fuzzer exp/c-ares/bin/fuzz/fuzzbuilder/ares_parse_aaaa_reply_fuzzer
afl-fuzz -m 1024 -i exp/c-ares/seed/fuzzbuilder/opt/ares_parse_a_reply_fuzzer -o exp/c-ares/output/fuzzbuilder/ares_parse_a_reply_fuzzer exp/c-ares/bin/fuzz/fuzzbuilder/ares_parse_a_reply_fuzzer
afl-fuzz -m 1024 -i exp/c-ares/seed/fuzzbuilder/opt/ares_parse_ptr_reply_fuzzer -o exp/c-ares/output/fuzzbuilder/ares_parse_ptr_reply_fuzzer exp/c-ares/bin/fuzz/fuzzbuilder/ares_parse_ptr_reply_fuzzer
afl-fuzz -m 1024 -i exp/c-ares/seed/fuzzbuilder/opt/ares_parse_ns_reply_fuzzer -o exp/c-ares/output/fuzzbuilder/ares_parse_ns_reply_fuzzer exp/c-ares/bin/fuzz/fuzzbuilder/ares_parse_ns_reply_fuzzer
afl-fuzz -m 1024 -i exp/c-ares/seed/fuzzbuilder/opt/ares_parse_srv_reply_fuzzer -o exp/c-ares/output/fuzzbuilder/ares_parse_srv_reply_fuzzer exp/c-ares/bin/fuzz/fuzzbuilder/ares_parse_srv_reply_fuzzer
afl-fuzz -m 1024 -i exp/c-ares/seed/fuzzbuilder/opt/ares_parse_mx_reply_fuzzer -o exp/c-ares/output/fuzzbuilder/ares_parse_mx_reply_fuzzer exp/c-ares/bin/fuzz/fuzzbuilder/ares_parse_mx_reply_fuzzer
afl-fuzz -m 1024 -i exp/c-ares/seed/fuzzbuilder/opt/ares_parse_txt_reply_fuzzer -o exp/c-ares/output/fuzzbuilder/ares_parse_txt_reply_fuzzer exp/c-ares/bin/fuzz/fuzzbuilder/ares_parse_txt_reply_fuzzer
afl-fuzz -m 1024 -i exp/c-ares/seed/fuzzbuilder/opt/ares_parse_soa_reply_fuzzer -o exp/c-ares/output/fuzzbuilder/ares_parse_soa_reply_fuzzer exp/c-ares/bin/fuzz/fuzzbuilder/ares_parse_soa_reply_fuzzer
afl-fuzz -m 1024 -i exp/c-ares/seed/fuzzbuilder/opt/ares_parse_naptr_reply_fuzzer -o exp/c-ares/output/fuzzbuilder/ares_parse_naptr_reply_fuzzer exp/c-ares/bin/fuzz/fuzzbuilder/ares_parse_naptr_reply_fuzzer
```
