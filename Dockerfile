FROM debian:stretch
MAINTAINER Joonun Jang <joonun.jang@gmail.com>
RUN apt-get -y update

# Install Basic Development Packages
RUN apt-get -y install git wget build-essential cmake gcc-multilib g++-multilib gnupg

# Install LLVM Development Package
RUN echo "#LLVM Repository" >> /etc/apt/sources.list
RUN echo "deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-6.0 main" >> /etc/apt/sources.list
RUN echo "deb-src http://apt.llvm.org/stretch/ llvm-toolchain-stretch-6.0 main" >> /etc/apt/sources.list
RUN wget -O ./key.gpg https://apt.llvm.org/llvm-snapshot.gpg.key && apt-key add < ./key.gpg && rm ./key.gpg
RUN apt-get -y update
RUN apt-get -y install clang-6.0 clang-6.0-dev llvm-6.0 llvm-6.0-dev
RUN ln -s /usr/bin/clang-6.0 /usr/bin/clang && ln -s /usr/bin/clang++-6.0 /usr/bin/clang++

# Install AFL
RUN mkdir -p tool
RUN cd /tool && wget http://lcamtuf.coredump.cx/afl/releases/afl-2.52b.tgz && tar zxvf afl-2.52b.tgz
RUN cd /tool/afl-2.52b && make

# Install FuzzBuilder
COPY src/build/fuzzbuilder /tool/

# Install cJSON
RUN mkdir -p exp/cJSON
RUN cd /exp/cJSON && git clone https://github.com/DaveGamble/cJSON.git seed
RUN cd /exp/cJSON/seed && git checkout 08103f048e5f54c8f60aaefda16761faf37114f2
RUN cd /exp/cJSON && cp -rf seed fuzz

# Build cJSON for fuzzing
RUN mkdir -p /exp/cJSON/fuzz/build
RUN cd /exp/cJSON/fuzz/build && cmake -DCMAKE_C_COMPILER=/tool/afl-2.52b/afl-clang -DCMAKE_C_COMPILER=/tool/afl-2.52b/afl-clang -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DCMAKE_C_FLAGS="-m32 -g -fsanitize=address" -DCMAKE_EXE_LINKER_FLAGS="-m32 -g -fsanitize=address" build .. && make

# Applying FuzzBuilder
COPY exp/cJSON/cJSON.conf /exp/cJSON/fuzz
RUN mkdir -p /exp/cJSON/fuzz/experiment/bin
RUN cd /exp/cJSON/fuzz/build/tests && AFL_USE_ASAN=1 /tool/afl-2.52b/afl-clang -emit-llvm -DCJSON_API_VISIBILITY -DCJSON_EXPORT_SYMBOLS -DENABLE_LOCALES -m32 -g -std=c89 -pedantic -Wall -Wextra -Werror -Wstrict-prototypes -Wwrite-strings -Wshadow -Winit-self -Wcast-align -Wformat=2 -Wmissing-prototypes -Wstrict-overflow=2 -Wcast-qual -Wundef -Wswitch-default -Wconversion -Wc++-compat -fstack-protector-strong -Wcomma -Wdouble-promotion -Wparentheses -Wunused-macros -Wmissing-variable-declarations -Wused-but-marked-unused -Wswitch-enum -fvisibility=hidden -o misc_tests.bc -c ../../tests/misc_tests.c && /tool/fuzzbuilder /exp/cJSON/fuzz/cJSON.conf && /tool/afl-2.52b/afl-clang -m32 -o misc_tests.o -c misc_tests.bc.mod.bc && AFL_USE_ASAN=1 /tool/afl-2.52b/afl-clang -m32 -g -std=c89 -pedantic -Wall -Wextra -Werror -Wstrict-prototypes -Wwrite-strings -Wshadow -Winit-self -Wcast-align -Wformat=2 -Wmissing-prototypes -Wstrict-overflow=2 -Wcast-qual -Wundef -Wswitch-default -Wconversion -Wc++-compat -fstack-protector-strong -Wcomma -Wdouble-promotion -Wparentheses -Wunused-macros -Wmissing-variable-declarations -Wused-but-marked-unused -Wswitch-enum -fvisibility=hidden misc_tests.o -o misc_tests -Wl,-rpath,$(realpath ../)  -rdynamic ./libunity.a -lm
