#!/bin/sh

#ARCH="amd64"
FLAVORS=("arch" "debian" "fedora" "ubuntu" "windows")
PLATFORM="linux/$ARCH"

# Build all in parallel
for OS in "${FLAVORS[@]}"; do
  (
      echo "Building $OS $PLATFORM.."
      sleep 1
      podman buildx build --platform $PLATFORM -t gbcemulator:$OS-$ARCH -f docker/$OS/Dockerfile .
  ) &
done
wait

echo "Finished building all"