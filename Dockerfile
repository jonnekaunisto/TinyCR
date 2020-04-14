FROM alpine:latest

RUN mkdir /TinyCR
ADD . /TinyCR
WORKDIR /TinyCR

RUN apk update && \
    apk upgrade && \
    apk --update add \
        gcc \
        g++ \
        build-base \
        cmake \
        bash \
        libstdc++ \
        cppcheck && \
    rm -rf /var/cache/apk/*

RUN cd /TinyCR/src \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make


CMD ["/TinyCR/src/build/server"]