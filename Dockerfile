FROM ubuntu:18.04

# docker build -t vanessa/halley .
# docker push vanessa/halley
# For video / audio
# singularity pull --name halley.simg docker://vanessa/halley

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get install -y g++ gcc \
                                         cmake clang-3.9 \
                                         wget \
                                         libfreetype6 \
                                         libsdl2-dev \
                                         software-properties-common \
                                         freetype2-demos \
                                         libfreetype6-dev

# above installs cmake 3.10.2, gcc 7.3.0

# Boost
RUN wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz && \
    tar -xzvf boost_1_66_0.tar.gz && \
    cd cd boost_1_66_0 && \
    ./bootstrap.sh && \
    ./b2 install

# boost header files in /usr/local/include/boost
# compiled libraries in /usr/local/lib

# yaml-cpp
RUN wget https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.5.3.tar.gz && \
    tar -xzvf yaml-cpp-0.5.3.tar.gz && \
    cd yaml-cpp-yaml-cpp-0.5.3 && \
    mkdir build && \ 
    cd build && \
    cmake -DBUILD_SHARED_LIBS=ON .. && \
    make && \
    make install

RUN mkdir -p /code

ADD . /code
WORKDIR /code

RUN cd /code/src && \
    cmake -DCMAKE_INCLUDE_PATH=/usr/local/lib -DHALLEY_PATH=../halley -DBUILD_HALLEY_TOOLS=1 -DBUILD_HALLEY_TESTS=0 -DCMAKE_LIBRARY_PATH=/usr/local/lib -DBOOST_ROOT=/usr/local/include/boost .. && \
    make

ENV PATH /code/bin:$PATH
# halley-cmd  halley-editor  halley-runner

# Run "halley-editor tests/entity" (or whichever other project you want to test)
# Launch that project
# The full documentation is available on the [Wiki](https://github.com/amzeratul/halley/wiki).

WORKDIR /code/src/tests
ENTRYPOINT ["halley-editor"]
