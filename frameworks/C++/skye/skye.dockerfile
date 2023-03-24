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

# Install conan package manager
RUN pip install conan --upgrade

# Copy repo source code
COPY ./src /source

# Run cmake commands from the build folder
WORKDIR /source/build

# Download dependencies and generate cmake toolchain file
RUN conan install .. --build=missing

# Configure
RUN cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Build
RUN cmake --build .

# Install
RUN cmake --install . --strip

FROM scratch as runtime

COPY --from=builder /usr/local/bin/skye_benchmark /skye

ENTRYPOINT ["/skye"]

EXPOSE 8080
