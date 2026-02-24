FROM ubuntu:25.10

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update -y
RUN apt install -y cmake gcc g++ git make pkg-config python3 python3-pip
RUN apt install -y libasound2-dev libpipewire-0.3-dev libpulse-dev perl
RUN apt install -y libpng-dev libzip-dev libspdlog-dev zlib1g-dev libsdl3-dev

WORKDIR /gbcemulator
COPY . .

RUN mkdir build && cd build && cmake .. && cmake --build .
RUN cd build && cmake --install . --prefix install
