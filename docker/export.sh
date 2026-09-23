#!/bin/sh

FLAVORS=("arch" "debian" "fedora" "ubuntu" "windows")

rm -rf export
mkdir export

echo 'Exporting amd64 to ./export'

# Export amd64 in parallel
for OS in "${FLAVORS[@]}"; do
  (
    cid=$(podman create gbcemulator:$OS-amd64) && podman cp "$cid:/export/." ./export/$OS && podman rm "$cid"
  ) &
done
wait

# Export arm64 in parallel
echo 'Exporting arm64 to ./export'
for OS in "${FLAVORS[@]}"; do
  (
    cid=$(podman create gbcemulator:$OS-arm64) && podman cp "$cid:/export/." ./export/$OS && podman rm "$cid"
  ) &
done
wait