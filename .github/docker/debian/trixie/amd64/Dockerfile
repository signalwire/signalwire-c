ARG BUILDER_IMAGE=debian:trixie-20250520

FROM ${BUILDER_IMAGE} AS builder

ARG MAINTAINER_NAME="Andrey Volk"
ARG MAINTAINER_EMAIL="andrey@signalwire.com"

ARG BUILD_NUMBER=42
ARG GIT_SHA=0000000000

ARG DATA_DIR=/data

LABEL maintainer="${MAINTAINER_NAME} <${MAINTAINER_EMAIL}>"

SHELL ["/bin/bash", "-c"]

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get -q update \
    && apt-get -y -q install \
        apt-transport-https \
        autoconf \
        automake \
        build-essential \
        ca-certificates \
        cmake \
        curl \
        debhelper \
        devscripts \
        dh-autoreconf \
        dos2unix \
        doxygen \
        dpkg-dev \
        git \
        gnupg2 \
        graphviz \
        libglib2.0-dev \
        libssl-dev \
        lsb-release \
        pkg-config \
        unzip \
        wget

RUN update-ca-certificates --fresh

RUN git config --global --add safe.directory '*' \
    && git config --global user.name "${MAINTAINER_NAME}" \
    && git config --global user.email "${MAINTAINER_EMAIL}"

RUN --mount=type=secret,id=REPO_PASSWORD,required=true \
    sha512sum /run/secrets/REPO_PASSWORD && \
    curl -sSL https://freeswitch.org/fsget | \
        bash -s $(cat /run/secrets/REPO_PASSWORD) prerelease && \
    apt-get --quiet update && \
    apt-get --yes --quiet install \
        libks2 \
    && rm -f /etc/apt/auth.conf

# Bootstrap and Build
COPY . ${DATA_DIR}
WORKDIR ${DATA_DIR}

RUN PACKAGE_RELEASE="${BUILD_NUMBER}.${GIT_SHA}" cmake . \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_INSTALL_PREFIX="/usr" \
    && make package \
    && mkdir OUT \
    && mv -v *.deb OUT/.

# Artifacts image (mandatory part, the resulting image must have a single filesystem layer)
FROM scratch
COPY --from=builder /data/OUT/ /
