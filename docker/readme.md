# Building with Docker/Podman

Building via `docker` or `podman` is available for Linux.

## Arch

`podman buildx build --platform linux/amd64 -t gbcemulator:arch-amd64 -f docker/arch/Dockerfile .`

## Debian

`podman build --platform linux/amd64 -t gbcemulator:debian-amd64 -f docker/debian/Dockerfile .`
`podman build --platform linux/arm64 -t gbcemulator:debian-arm64 -f docker/debian/Dockerfile .`

`podman build --platform linux/amd64,linux/arm64 -t gbcemulator:debian -f docker/debian/Dockerfile .`

## Fedora

`podman build --platform linux/amd64 -t gbcemulator:fedora-amd64 -f docker/fedora/Dockerfile .`
`podman build --platform linux/arm64 -t gbcemulator:fedora-arm64 -f docker/fedora/Dockerfile .`

`podman build --platform linux/amd64,linux/arm64 -t gbcemulator:fedora -f docker/fedora/Dockerfile .`

## Ubuntu

`podman build --platform linux/amd64 -t gbcemulator:ubuntu-amd64 -f docker/ubuntu/Dockerfile .`
`podman build --platform linux/arm64 -t gbcemulator:ubuntu-arm64 -f docker/ubuntu/Dockerfile .`

`podman build --platform linux/amd64,linux/arm64 -t gbcemulator:ubuntu -f docker/ubuntu/Dockerfile .`

## Windows

`podman buildx build --platform linux/amd64 -t gbcemulator:windows -f docker/windows/Dockerfile .`


# Exporting

cid=$(podman create gbcemulator:debian) && podman cp "$cid:/export/." ./export/debian && podman rm "$cid"
cid=$(podman create gbcemulator:ubuntu) && podman cp "$cid:/export/." ./export/ubuntu && podman rm "$cid"
cid=$(podman create gbcemulator:fedora) && podman cp "$cid:/export/." ./export/fedora && podman rm "$cid"
cid=$(podman create gbcemulator:arch)   && podman cp "$cid:/export/." ./export/arch   && podman rm "$cid"