ARG LIBKS_IMAGE=signalwire/freeswitch-libs:libks-libs10
ARG BASEIMAGE=signalwire/freeswitch-base:debian-10
FROM ${LIBKS_IMAGE} as libks
FROM ${BASEIMAGE}

COPY REVISION /REVISION
COPY --from=libks /usr/lib/libks2.so /usr/lib/libks2.so
COPY --from=libks /usr/lib/libks2.so.2 /usr/lib/libks2.so.2
COPY --from=libks /usr/lib/pkgconfig/libks2.pc /usr/lib/pkgconfig/libks2.pc
COPY --from=libks /usr/include/libks2 /usr/include/libks2
RUN mkdir -p /sw/signalwire-c
WORKDIR /sw/signalwire-c
COPY . .

RUN PKG_CONFIG_PATH=/usr/lib/pkgconfig cmake . -DCMAKE_INSTALL_PREFIX=/usr && make install