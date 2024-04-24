ARG BUILDER_IMAGE=arm64v8/debian:buster
ARG MAINTAINER="Andrey Volk <andrey@signalwire.com>"

FROM ${BUILDER_IMAGE} AS builder

# Credentials
ARG REPO_DOMAIN=freeswitch.signalwire.com
ARG REPO_USERNAME=user
ARG REPO_PASSWORD=password

ARG BUILD_NUMBER=42
ARG GIT_SHA=0000000000

ARG GPG_KEY="/usr/share/keyrings/signalwire-freeswitch-repo.gpg"

MAINTAINER ${MAINTAINER}

SHELL ["/bin/bash", "-c"]

RUN apt-get -q update && \
    DEBIAN_FRONTEND=noninteractive apt-get -yq install \
        apt-transport-https \
        build-essential \
        ca-certificates \
        cmake \
        curl \
        debhelper \
        devscripts \
        dh-autoreconf \
        dos2unix \
        doxygen \
        git \
        graphviz \
        libglib2.0-dev \
        libssl-dev \
        lsb-release \
        pkg-config \
        wget

RUN update-ca-certificates --fresh

ENV DATA_DIR=/data
WORKDIR ${DATA_DIR}

COPY . ${DATA_DIR}
RUN git reset --hard HEAD && git clean -xfd

RUN echo "export CODENAME=$(lsb_release -sc | tr -d '\n')" | tee ~/.env && \
    chmod +x ~/.env

RUN . ~/.env && cat <<EOF > /etc/apt/sources.list.d/freeswitch.list
deb [signed-by=${GPG_KEY}] https://${REPO_DOMAIN}/repo/deb/debian-release ${CODENAME} main
deb-src [signed-by=${GPG_KEY}] https://${REPO_DOMAIN}/repo/deb/debian-release ${CODENAME} main
EOF

RUN --mount=type=secret,id=REPO_PASSWORD \
  printf "machine ${REPO_DOMAIN} " > ~/.netrc && \
  printf "login ${REPO_USERNAME} " >> ~/.netrc && \
  printf "password " >> ~/.netrc && \
  cat /run/secrets/REPO_PASSWORD >> ~/.netrc && \
  cp -f ~/.netrc /etc/apt/auth.conf

RUN git config --global --add safe.directory '*'

RUN curl --netrc -o ${GPG_KEY} https://${REPO_DOMAIN}/repo/deb/debian-release/signalwire-freeswitch-repo.gpg

# Bootstrap and Build
RUN apt-get -q update && \
    DEBIAN_FRONTEND=noninteractive apt-get -yq install \
        libks2

RUN PACKAGE_RELEASE="${BUILD_NUMBER}.${GIT_SHA}" cmake . \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="/usr" && \
    make package && mkdir OUT && mv -v *.deb OUT/.

# Artifacts image (mandatory part, the resulting image must have a single filesystem layer)
FROM scratch
COPY --from=builder /data/OUT/ /
