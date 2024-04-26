ARG BUILDER_IMAGE=centos:7.2.1511

FROM ${BUILDER_IMAGE} AS builder

# Credentials
ARG REPO_DOMAIN=fsa.freeswitch.com
ARG REPO_USERNAME=user
ARG REPO_PASSWORD=password

ARG GPG_KEY="/etc/pki/rpm-gpg/RPM-GPG-KEY-FREESWITCH"
ARG GPG_KEY_URL="https://files.freeswitch.org/repo/yum/centos-release/RPM-GPG-KEY-FREESWITCH"

ARG MAINTAINER="Andrey Volk <andrey@signalwire.com>"
ARG BUILD_NUMBER=42
ARG GIT_SHA=0000000000

MAINTAINER ${MAINTAINER}
ENV MAINTAINER=${MAINTAINER}

RUN yum -y update && \
	yum install -y \
		audiofile-devel \
		autoconf \
		automake \
		bind-license \
		centos-release-scl \
		cyrus-sasl-lib \
		dbus \
		dbus-libs \
		devtoolset-9-gcc* \
		dos2unix \
		doxygen \
		dpkg-dev \
		dracut \
		epel-release \
		expat \
		gcc \
		gcc-c++ \
		git \
		glib2 \
		glib2-devel \
		gnupg2 \
		gzip \
		krb5-dxevel \
		libatomic \
		libcurl-devel \
		libtool \
		libuuid-devel \
		libxml2 \
		lksctp-tools-devel \
		lsb_release \
		make \
		multilib-rpm-config \
		openssl-devel \
		pkg-config \
		procps-ng \
		python \
		python-libs \
		rpm-build \
		rpmdevtools \
		scl-utils \
		sqlite \
		swig \
		unzip \
		uuid-devel \
		vim-minimal \
		wget \
		which \
		xz \
		xz-libs \
		yum-plugin-fastestmirror \
		yum-plugin-ovl \
		yum-utils \
		zlib-devel && \
	yum -y clean all

ENV CMAKE_VERSION 3.22.2

RUN set -ex \
	&& curl -kfsSLO --compressed https://cmake.org/files/v3.22/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz \
	&& curl -kfsSLO --compressed https://cmake.org/files/v3.22/cmake-${CMAKE_VERSION}-SHA-256.txt \
	&& grep "cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz\$" cmake-${CMAKE_VERSION}-SHA-256.txt | sha256sum -c - \
	&& tar xzf cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz -C /usr/local --strip-components=1 --no-same-owner \
	&& rm -rf cmake-${CMAKE_VERSION}* \
	&& cmake --version

ENV DATA_DIR=/data
WORKDIR ${DATA_DIR}

COPY . ${DATA_DIR}/
RUN cd ${DATA_DIR}/ && \
	git reset --hard HEAD && git clean -xfd && \
	cd ${DATA_DIR}

RUN cat <<EOF > /etc/yum.repos.d/freeswitch.repo
[freeswitch]
name=FreeSWITCH Packages for Enterprise Linux \$releasever - \$basearch
baseurl=https://freeswitch.signalwire.com/repo/yum/centos-release/\$releasever/\$basearch
enabled=1
gpgcheck=0
gpgkey=file://${GPG_KEY}
username=\$repousername
password=\$repopassword

[freeswitch-debuginfo]
name=FreeSWITCH Packages for Enterprise Linux \$releasever - \$basearch - Debug
baseurl=https://freeswitch.signalwire.com/repo/yum/centos-release/\$releasever/\$basearch
enabled=1
gpgkey=file://${GPG_KEY}
gpgcheck=0
username=\$repousername
password=\$repopassword

[freeswitch-source]
name=FreeSWITCH Packages for Enterprise Linux \$releasever - \$basearch - Source
baseurl=https://freeswitch.signalwire.com/repo/yum/centos-release/\$releasever/\$basearch
enabled=1
gpgkey=file://${GPG_KEY}
gpgcheck=0
username=\$repousername
password=\$repopassword
EOF

RUN --mount=type=secret,id=REPO_PASSWORD \
	printf "machine ${REPO_DOMAIN} " > ~/.netrc && \
	printf "login ${REPO_USERNAME} " >> ~/.netrc && \
	printf "password " >> ~/.netrc && \
	cat /run/secrets/REPO_PASSWORD >> ~/.netrc && \
	printf "${REPO_USERNAME}" > /etc/yum/vars/repousername && \
	cat /run/secrets/REPO_PASSWORD > /etc/yum/vars/repopassword

RUN curl --netrc -o ${GPG_KEY} ${GPG_KEY_URL}

# Bootstrap and Build
RUN yum -y update && \
	yum install -y \
		libks2 \
	&& \
	yum -y clean all

RUN PACKAGE_RELEASE="${BUILD_NUMBER}.${GIT_SHA}" cmake . \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_INSTALL_PREFIX="/usr" && \
	make package && mkdir OUT && mv -v *.rpm OUT/.

# Artifacts image (mandatory part, the resulting image must have a single filesystem layer)
FROM scratch
COPY --from=builder /data/OUT/ /
