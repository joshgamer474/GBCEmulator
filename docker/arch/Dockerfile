FROM archlinux:latest

RUN pacman -Syu --noconfirm
RUN pacman -Sy --noconfirm cmake gcc git make pkgconf python python-pip
RUN pacman -Sy --noconfirm alsa-lib pipewire libpulse perl
RUN pacman -Sy --noconfirm libpng libzip spdlog zlib sdl3

WORKDIR /gbcemulator
COPY . .

# Build
RUN mkdir build && cd build && cmake .. && cmake --build .
# Install
RUN cd build && cmake --install . --prefix install
# Package
RUN cd build && cpack --config CPackConfig.cmake -G TGZ
# Export
RUN mkdir -p /export && mv build/*.tar.gz /export/
