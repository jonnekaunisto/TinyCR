FROM ubuntu:16.04

RUN mkdir /TinyCR
ADD . /TinyCR
WORKDIR /TinyCR

RUN apt-get update && apt-get install -y build-essential \
    sudo \
    clang-3.6 \
    clang-format-3.6 \
    wget \
    git

RUN cd /usr/local/src \ 
    && wget https://cmake.org/files/v3.4/cmake-3.4.3.tar.gz \
    && tar xvf cmake-3.4.3.tar.gz \ 
    && cd cmake-3.4.3 \
    && ./bootstrap \
    && make \
    && make install \
    && cd .. \
    && rm -rf cmake*


#RUN cd /usr/local/src \
#    && mkdir build \
#    && cd build \
