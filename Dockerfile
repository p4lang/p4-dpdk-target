# Share args between stages.
# See https://docs.docker.com/engine/reference/builder/#understand-how-arg-and-from-interact.
# They should be consistent with tools/setup/p4sde_env_setup.sh.
ARG SDE=/home/sde
ARG SDE_INSTALL=${SDE}/install
ARG LD_LIBRARY_PATH=${SDE_INSTALL}/lib:${SDE_INSTALL}/lib64:${SDE_INSTALL}/lib/x86_64-linux-gnu/
ARG LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib64
ARG LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib

#======================================
# Stage 1/3: Reuse upstream p4c image.
#======================================
# FROM p4lang/p4c:latest as p4c
# :latest on 2023-06-23:
FROM p4lang/p4c@sha256:c1cbb66cea83de50b43d7ef78d478dd5e43ce9e1116921d6700cc40bb505e12a as p4c

#==============================================
# Stage 2/3: Build P4 DPDK target from source.
#==============================================
FROM ubuntu:20.04 as p4-dpdk-target
ARG DEBIAN_FRONTEND=noninteractive
ARG SDE
ARG SDE_INSTALL
ARG LD_LIBRARY_PATH

# Install core packages needed for later steps.
RUN apt-get update && apt-get install -y \
    python-is-python3 \
    python3 \
    python3-pip \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# Copy repo files in.
ARG REPO_NAME=p4-dpdk-target
COPY . ${SDE}/${REPO_NAME}/

# Install dependent packages.
WORKDIR ${SDE}/${REPO_NAME}/tools/setup
RUN pip3 install distro \
    && apt-get update \
    && python3 install_dep.py \
    && rm -rf /var/lib/apt/lists/*

# Build P4 DPDK target.
WORKDIR ${SDE}/${REPO_NAME}
RUN ./autogen.sh \
    && ./configure --prefix=${SDE_INSTALL} \
    && make -j \
    && make install

#===================================
# Stage 3/3: Construct final image.
#===================================
FROM ubuntu:20.04
LABEL maintainer="P4 Developers <p4-dev@lists.p4.org>"
ARG DEBIAN_FRONTEND=noninteractive
ARG SDE
ARG SDE_INSTALL
ARG LD_LIBRARY_PATH

# Persist essential environment variables.
ENV SDE=${SDE}
ENV SDE_INSTALL=${SDE_INSTALL}
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}

# Install some useful tools.
RUN apt-get update && apt-get install -y \
    iproute2 \
    python-is-python3 \
    python3 \
    python3-pip \
    sudo \
    && rm -rf /var/lib/apt/lists/*
RUN pip3 install --upgrade \
    scapy

# [from p4c]
# Just copy minimal p4c bins & libs for dpdk backend.
# Adapted from https://github.com/sonic-net/DASH/blob/main/dash-pipeline/dockerfiles/Dockerfile.p4c-dpdk.
COPY --from=p4c \
    /usr/local/bin/p4c \
    /usr/local/bin/p4c-dpdk \
    /usr/local/bin/
COPY --from=p4c \
    /usr/lib/x86_64-linux-gnu/libboost_*so* \
    /usr/lib/x86_64-linux-gnu/libgc.so* \
    /usr/lib/x86_64-linux-gnu/
COPY --from=p4c \
    /usr/local/share/p4c/ \
    /usr/local/share/p4c/

# [from p4-dpdk-target]
# Just copy SDE_INSTALL dir & minimal system libs.
COPY --from=p4-dpdk-target \
    ${SDE_INSTALL} \
    ${SDE_INSTALL}
COPY --from=p4-dpdk-target \
    /usr/lib/x86_64-linux-gnu/libedit.so* \
    /usr/lib/x86_64-linux-gnu/

# Set default working directory.
WORKDIR ${SDE}
