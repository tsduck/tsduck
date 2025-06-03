# Using TSDuck with Docker Containers

## Sample Docker packaging

The `Dockerfile` in this directory builds a container image named `tsduck`.
This image is based on Alpine Linux, a lightweigth Linux distribution which
is commonly used in containers.

In the `tsduck` container, TSDuck is installed in the default system locations.
All usual TSDuck commands and plugins are available.

Because containers are usually not able to access host devices, support for
external devices is removed: smartcards (PCSC), Dektec, Vatek, and HiDes devices.

Other forms of TSDuck containers can be built based on this example.
Please note the following:

- To listen on incoming UDP or TCP ports, do not forget to add the corresponding
  `EXPOSE` directives in `Dockerfile`.
- If you find a way to use Dektec devices in the container, use another base
  container image, for instance `ubuntu`. Alpine Linux is not able to support
  Dektec devices because of the musl libc which is used instead of glibc.e

## Using Docker to build TSDuck for various Linux distros

The various scripts `build-tsduck-XXX.sh` illustrate how to build the TSDuck
binary packages for various Linux distros using standard containers, all from
the same Linux host.

## Where to find base images for containers

The list of available containers, in the operating systems category, is here:
https://hub.docker.com/search?categories=Operating+Systems

As of this writing, the available images for the most common Linux distros are
summarized in the following table.

| Distro   | Image name        | Equivalent  | Architectures
| -------- | ----------------- | ----------- | -------------
| Ubuntu   | ubuntu:latest     | 24.04       | x64, arm64
|          | ubuntu:25.04      |             | x64, arm64
|          | ubuntu:24.04      |             | x64, arm64
| Debian   | debian:latest     | 12          | x64, arm64
|          | debian:trixie     |             | x64, arm64
|          | debian:12         |             | x64, arm64
| Fedora   | fedora:latest     | 42          | x64, arm64
|          | fedora:42         |             | x64, arm64
|          | fedora:41         |             | x64, arm64
| Red Hat  | almalinux:latest  | 9           | x64, arm64
|          | almalinux:10      |             | x64, arm64
|          | almalinux:9       |             | x64, arm64
|          | rockylinux:9      | (no latest) | x64, arm64
| Alpine   | alpine:latest     |             | x64, arm64
| Arch     | archlinux:latest  |             | x64
