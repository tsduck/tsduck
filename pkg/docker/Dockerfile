#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Dockerfile for a generic TSDuck container (no external device support).
#  Requires archive tsduck.tgz in current directory (see build.sh).
#
#-----------------------------------------------------------------------------

FROM alpine as builder

RUN apk update && \
    apk upgrade --no-interactive --no-progress && \
    apk add --no-interactive --no-progress \
        bash coreutils diffutils procps util-linux linux-headers \
        git make g++ dos2unix curl tar zip python3 \
        openssl-dev libedit-dev zlib-dev librist-dev libsrt-dev curl-dev \
        asciidoctor qpdf && \
    gem install asciidoctor-pdf rouge

ENV LANG en_US.UTF-8
ENV LC_ALL en_US.UTF-8

WORKDIR /tmp/tsduck
COPY tsduck.tgz .

RUN tar -xpzf tsduck.tgz && \
    make NOPCSC=1 NODEKTEC=1 NOVATEK=1 NOHIDES=1 NOJAVA=1 NOTEST=1 -j8 && \
    make NOPCSC=1 NODEKTEC=1 NOVATEK=1 NOHIDES=1 NOJAVA=1 NOTEST=1 install-tools SYSROOT=/tmp/root

FROM alpine

RUN apk update && \
    apk upgrade --no-interactive --no-progress && \
    apk add --no-interactive --no-progress libstdc++ libedit libcurl librist libsrt

ENV LANG en_US.UTF-8
ENV LC_ALL en_US.UTF-8

COPY --from=builder /tmp/root/ /
