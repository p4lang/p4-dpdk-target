ARG BASE_IMAGE=p4lang/p4c:latest
FROM ${BASE_IMAGE}
LABEL maintainer="P4 Developers <p4-dev@lists.p4.org>"

# Install some packages needed for later steps
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    python-is-python3 \
    python3 \
    python3-pip \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# Set environment variables
# We don't simply source tools/setup/p4sde_env_setup.sh,
# because we want to persist these in the running container.
ENV SDE=/home/sde
ENV SDE_INSTALL=${SDE}/install
ENV LD_LIBRARY_PATH=${SDE_INSTALL}/lib:${SDE_INSTALL}/lib64:${SDE_INSTALL}/lib/x86_64-linux-gnu/
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib64
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib

# Copy repo files in
ARG REPO_NAME=p4-dpdk-target
COPY . ${SDE}/${REPO_NAME}/

# Install dependent packages
WORKDIR ${SDE}/${REPO_NAME}/tools/setup
RUN pip3 install distro \
    && apt-get update \
    && python3 install_dep.py \
    && rm -rf /var/lib/apt/lists/*

# Build P4 DPDK target
WORKDIR ${SDE}/${REPO_NAME}
RUN ./autogen.sh \
    && ./configure --prefix=${SDE_INSTALL} \
    && make -j \
    && make install

# Set working directory
WORKDIR ${SDE}
