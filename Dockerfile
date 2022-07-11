# special thanks to AluFers

FROM  --platform=$TARGETPLATFORM  ubuntu:22.10 AS builder

RUN apt-get update -y && apt-get install -y gcc-aarch64-linux-gnu \
    libboost-all-dev libmodbus-dev libmosquitto-dev libmosquittopp-dev libyaml-cpp-dev rapidjson-dev \
    build-essential cmake catch2 pkg-config

ARG TARGETPLATFORM
ARG BUILDPLATFORM

COPY . /app

WORKDIR /app

RUN if [ "$TARGETPLATFORM" -eq "linux/arm64" ]; then export CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++ ; fi \
    && mkdir -p build && cd build && cmake .. && make -j$(nproc)


FROM --platform=$TARGETPLATFORM  ubuntu:22.10 AS target
COPY --from=builder /app/build/modmqttd/modmqttd /app/modmqttd
COPY --from=builder /app/build/libmodmqttsrv/libmodmqttsrv.so /app/libmodmqttsrv.so
RUN ln -s /app/libmodmqttsrv.so /usr/lib/libmodmqttsrv.so
RUN ln -s /app/libmodmqttsrv.so /usr/lib/libmodmqttsrv.so.1

RUN apt-get update -y && apt-get install -y --no-upgrade libmosquitto1 libmodbus5 libyaml-cpp0.7 #libboost-log1.74 # boost1.74 

CMD ["/app/modmqttd"]
