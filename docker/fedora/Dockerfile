FROM fedora:44

RUN dnf update -y
RUN dnf install -y cmake gcc gcc-c++ git make pkgconfig python3 python3-pip
RUN dnf install -y alsa-lib-devel pipewire-devel pulseaudio-libs-devel perl
RUN dnf install -y libpng-devel libzip-devel spdlog-devel zlib-ng-devel zlib-ng-compat-devel zlib-ng-compat-static SDL3-devel
RUN dnf install -y file rpm-build

WORKDIR /gbcemulator
COPY . .

# Build
RUN mkdir build && cd build && cmake .. && cmake --build .
# Install
RUN cd build && cmake --install . --prefix install
# Package
RUN cd build && cpack --config CPackConfig.cmake -G RPM
# Export
RUN mkdir -p /export && mv build/*.rpm /export/
