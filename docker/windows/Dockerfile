FROM archlinux:latest

RUN pacman -Syu --noconfirm
RUN pacman -Sy --noconfirm cmake gcc git make pkgconf
RUN pacman -Sy --noconfirm curl mingw-w64-gcc zip unzip tar

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg && /opt/vcpkg/bootstrap-vcpkg.sh

# Install dependencies via vcpkg
ARG VCPKG=/opt/vcpkg/vcpkg
ARG TARGET_TRIPLET=x64-mingw-static
RUN $VCPKG install zlib:$TARGET_TRIPLET
RUN $VCPKG install libpng:$TARGET_TRIPLET
RUN $VCPKG install libzip:$TARGET_TRIPLET
RUN $VCPKG install spdlog:$TARGET_TRIPLET
RUN $VCPKG install sdl3:$TARGET_TRIPLET

WORKDIR /gbcemulator
COPY . .

# Configure
ARG MINGW_TOOLCHAIN=/gbcemulator/cmake/mingw-toolchain.cmake
ARG VCPKG_TOOLCHAIN=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
ARG ARGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_TOOLCHAIN -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$MINGW_TOOLCHAIN -DVCPKG_TARGET_TRIPLET=$TARGET_TRIPLET"
RUN mkdir build && cd build && cmake $ARGS ..
# Build
RUN cd build && cmake --build . -j 12
# Install
RUN cd build && cmake --install . --prefix install
# Package
RUN cd build && cpack --config CPackConfig.cmake -G ZIP
# Export
RUN mkdir -p /export && mv build/*.zip /export/
