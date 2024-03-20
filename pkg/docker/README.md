# Sample Docker packaging

This `Dockerfile` builds a container image named `tsduck`. This image is based on
Alpine Linux, a lightweigth Linux distribution which is commonly used in containers.

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
  Dektec devices because of the musl libc which is used instead of glibc.
