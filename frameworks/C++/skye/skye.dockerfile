# Build with static musl/libc and libstdc++ to run on scratch image
FROM alpine:3.17 as builder

# Install build requirements from package repo
RUN apk update && apk add --no-cache \
    cmake \
    g++ \
    git \
    linux-headers \
    make \
    ninja \
    py-pip

RUN pip install conan && conan profile detect

RUN git clone https://github.com/luketokheim/skye /skye
WORKDIR skye
RUN cat tools/standalone.conf >> ~/.conan2/global.conf
RUN conan create . --build=missing

COPY ./src /source
WORKDIR /source
RUN conan install . --build=missing

WORKDIR /source/build/Release
RUN cmake ../.. -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake

# Build
RUN cmake --build .

# Install
RUN cmake --install . --strip

FROM scratch as runtime

COPY --from=builder /usr/local/bin/skye_benchmark /skye

ENTRYPOINT ["/skye"]

EXPOSE 8080
