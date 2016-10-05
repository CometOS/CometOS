#
# CometOS Dockerfile
#
# Based on the RIOT-OS Dockerfile by Joakim Nohlgård <joakim.nohlgard@eistec.se>
#
# Setup: (only needed once per Dockerfile change)
# 1. install docker, add yourself to docker group, enable docker, relogin
# 2. # docker build -t cometosbuild .
#
# Usage:
# 3. set COMETOS_PATH environment variable to the CometOS root
# 4. docker run -i -t -e LOCAL_USER_ID=$UID --device=/dev/bus/usb:/dev/bus/usb:rwm -v $COMETOS_PATH:/data/cometosbuild -w /data/cometosbuild/examples/hardware/examples/blink cometosbuild cob platform=AutoRNode go

FROM ubuntu:xenial

MAINTAINER Florian Kauer <florian.kauer@koalo.de>

ENV DEBIAN_FRONTEND noninteractive

# Install gosu (for running commands as other users and to handle signals correctly)
ENV GOSU_VERSION 1.9
RUN set -x \
    && apt-get update && apt-get install -y --no-install-recommends ca-certificates wget && rm -rf /var/lib/apt/lists/* \
    && dpkgArch="$(dpkg --print-architecture | awk -F- '{ print $NF }')" \
    && wget -O /usr/local/bin/gosu "https://github.com/tianon/gosu/releases/download/$GOSU_VERSION/gosu-$dpkgArch" \
    && wget -O /usr/local/bin/gosu.asc "https://github.com/tianon/gosu/releases/download/$GOSU_VERSION/gosu-$dpkgArch.asc" \
    && export GNUPGHOME="$(mktemp -d)" \
    && gpg --keyserver ha.pool.sks-keyservers.net --recv-keys B42F6819007F00F88E364FD4036A9C25BF357DD4 \
    && gpg --batch --verify /usr/local/bin/gosu.asc /usr/local/bin/gosu \
    && rm -r "$GNUPGHOME" /usr/local/bin/gosu.asc \
    && chmod +x /usr/local/bin/gosu \
    && gosu nobody true \
    && apt-get purge -y --auto-remove ca-certificates wget

# The following package groups will be installed:
# - upgrade all system packages to latest available version
# - native platform development and build system functionality
# - Cortex-M development (about 550 MB installed), through the gcc-arm-embedded PPA
# - AVR development (about 110 MB installed)
RUN \
    echo 'Adding gcc-arm-embedded PPA' >&2 && \
    echo "deb http://ppa.launchpad.net/team-gcc-arm-embedded/ppa/ubuntu xenial main" \
     > /etc/apt/sources.list.d/gcc-arm-embedded.list && \
    apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 \
    --recv-keys B4D03348F75E3362B1E1C2A1D1FAA6ECF64D33B0 && \
    echo 'Upgrading all system packages to the latest available versions' >&2 && \
    apt-get update && apt-get -y dist-upgrade \
    && echo 'Installing native toolchain and build system functionality' >&2 && \
    apt-get -y install \
        build-essential \
        doxygen \
        gdb \
        git \
        python \
        python3 \
        vim \
        wget \
        scons \
        python-psutil \
        usbutils \
    && echo 'Installing Cortex-M toolchain' >&2 && \
    apt-get -y install \
        gcc-arm-embedded \
    && echo 'Installing AVR toolchain' >&2 && \
    apt-get -y install \
        gcc-avr \
        binutils-avr \
        avrdude \
        avr-libc \
    && echo 'Cleaning up installation files' >&2 && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
    git config --system user.name "CometOS" && \
    git config --system user.email "cometos@example.com" && \
    mkdir -p /data/cometosbuild 
WORKDIR /data/cometosbuild

# Copy our entry point script (signal wrapper)
COPY entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/bin/bash","/entrypoint.sh"]

# Set CometOS environment variables
ENV COMETOS_PATH /data/cometosbuild

# By default, run a shell when no command is specified on the docker command line
CMD ["/bin/bash"]


