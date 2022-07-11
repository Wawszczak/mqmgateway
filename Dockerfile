# special thanks to AluFers

FROM  --platform=$TARGETPLATFORM  alpine:3.16 AS builder

RUN apk add --no-cache build-base catch2 cmake make gcc pkgconfig libmodbus-dev mosquitto-dev rapidjson-dev yaml-cpp-dev

ARG TARGETPLATFORM
ARG BUILDPLATFORM

COPY . /app

WORKDIR /app
ENV CMAKE_CXX_COMPILER g++
ENV CMAKE_C_COMPILER gcc

RUN mkdir -p /usr/local/include && wget https://raw.githubusercontent.com/ArashPartow/exprtk/master/exprtk.hpp -O /usr/local/include/exprtk.hpp
RUN mkdir -p build && cd build && cmake .. && make -j$(nproc)

FROM --platform=$TARGETPLATFORM  alpine:3.16 AS target
COPY --from=builder /app/build/modmqttd/modmqttd /app/modmqttd
COPY --from=builder /app/build/exprconv.so /app/exprconv.so
COPY --from=builder /app/build/stdconv.so /app/stdconv.so
COPY --from=builder /app/build/libmodmqttsrv/libmodmqttsrv.so /app/libmodmqttsrv.so
RUN ln -s /app/libmodmqttsrv.so /usr/lib/libmodmqttsrv.so
RUN ln -s /app/libmodmqttsrv.so /usr/lib/libmodmqttsrv.so.1

RUN apk add --no-cache \
    libstdc++ \
    libgcc \
    libmodbus \
    mosquitto \
    rapidjson \
    yaml-cpp 

CMD ["/app/modmqttd"]
