FROM archlinux:latest

RUN pacman -Syu --noconfirm
RUN pacman -Sy --noconfirm cmake gcc git make pkgconf python python-pip
RUN pacman -Sy --noconfirm alsa-lib pipewire libpulse perl
RUN pacman -Sy --noconfirm libpng libzip spdlog zlib sdl3

WORKDIR /gbcemulator
COPY . .

RUN mkdir build && cd build && cmake .. && cmake --build .
RUN cd build && cmake --install . --prefix install
