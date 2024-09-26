ARG BUILDER_IMAGE=arm32v7/debian:bullseye-20240513

FROM --platform=linux/arm/v7 ${BUILDER_IMAGE} AS builder

ARG MAINTAINER_NAME="Andrey Volk"
ARG MAINTAINER_EMAIL="andrey@signalwire.com"

ARG CODENAME=bullseye
ARG ARCH=arm32

# Credentials
ARG REPO_DOMAIN=freeswitch.signalwire.com
ARG REPO_USERNAME=user

ARG BUILD_NUMBER=42
ARG GIT_SHA=0000000000

ARG GPG_KEY="/usr/share/keyrings/signalwire-freeswitch-repo.gpg"

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

RUN echo "export CODENAME=${CODENAME}" | tee ~/.env \
    && echo "export ARCH=${ARCH}" | tee -a ~/.env \
    && chmod +x ~/.env

RUN . ~/.env && cat <<EOF > /etc/apt/sources.list.d/freeswitch.list
deb [signed-by=${GPG_KEY}] https://${REPO_DOMAIN}/repo/deb/rpi/debian-release ${CODENAME} main
deb-src [signed-by=${GPG_KEY}] https://${REPO_DOMAIN}/repo/deb/rpi/debian-release ${CODENAME} main
EOF

RUN git config --global --add safe.directory '*' \
    && git config --global user.name "${MAINTAINER_NAME}" \
    && git config --global user.email "${MAINTAINER_EMAIL}"

RUN --mount=type=secret,id=REPO_PASSWORD,required=true \
    printf "machine ${REPO_DOMAIN} "  > /etc/apt/auth.conf && \
    printf "login ${REPO_USERNAME} " >> /etc/apt/auth.conf && \
    printf "password "               >> /etc/apt/auth.conf && \
    cat /run/secrets/REPO_PASSWORD   >> /etc/apt/auth.conf && \
    sha512sum /run/secrets/REPO_PASSWORD && \
    curl \
        --fail \
        --netrc-file /etc/apt/auth.conf \
        --output ${GPG_KEY} \
        https://${REPO_DOMAIN}/repo/deb/rpi/debian-release/signalwire-freeswitch-repo.gpg && \
    file ${GPG_KEY} && \
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
