FROM debian:trixie-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update -y
RUN apt install -y cmake gcc g++ git make pkg-config python3 python3-pip
RUN apt install -y libasound2-dev libpipewire-0.3-dev libpulse-dev perl
RUN apt install -y libpng-dev libzip-dev libspdlog-dev zlib1g-dev libsdl3-dev
RUN apt install -y file dpkg-dev

WORKDIR /gbcemulator
COPY . .

# Build
RUN mkdir build && cd build && cmake .. && cmake --build .
# Install
RUN cd build && cmake --install . --prefix install
# Package
RUN cd build && cpack --config CPackConfig.cmake -G DEB
# Export
RUN mkdir -p /export && mv build/*.deb /export/
