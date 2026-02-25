#!/bin/sh

rm -rf export
mkdir export

echo 'Exporting amd64 to ./export'
cid=$(podman create gbcemulator:debian-amd64) && podman cp "$cid:/export/." ./export/debian && podman rm "$cid"
cid=$(podman create gbcemulator:ubuntu-amd64) && podman cp "$cid:/export/." ./export/ubuntu && podman rm "$cid"
cid=$(podman create gbcemulator:fedora-amd64) && podman cp "$cid:/export/." ./export/fedora && podman rm "$cid"

echo 'Exporting arm64 to ./export'
cid=$(podman create gbcemulator:debian-arm64) && podman cp "$cid:/export/." ./export/debian && podman rm "$cid"
cid=$(podman create gbcemulator:ubuntu-arm64) && podman cp "$cid:/export/." ./export/ubuntu && podman rm "$cid"
cid=$(podman create gbcemulator:fedora-arm64) && podman cp "$cid:/export/." ./export/fedora && podman rm "$cid"

#cid=$(podman create --platform=linux/amd64 gbcemulator:arch)   && podman cp "$cid:/export/." ./export/arch   && podman rm "$cid"