FROM ubuntu:22.04

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install basic dependencies
RUN apt-get update && apt-get install -y \
    curl \
    ca-certificates \
    git \
    python3-pip \
    python3-venv \ 
    nodejs \
    npm \
    libsdl1.2debian \
    libsdl2-dev \
    libfdt1 \
    build-essential \
    wget \
    sudo \
    gh \
    clang-format \
    && rm -rf /var/lib/apt/lists/*

# Create vscode user (same as in MS devcontainers)
RUN useradd -m vscode && \
    usermod -aG sudo vscode && \
    echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN mkdir -p /usr/local/uv \
    && export UV_INSTALL_DIR=/usr/local/uv \
    && export UV_UNMANAGED_INSTALL=/usr/local/uv \
    && curl -LsSf https://astral.sh/uv/install.sh | sh \
    && ln -s /usr/local/uv/uv /usr/local/bin/uv \
    && ln -s /usr/local/uv/uvx /usr/local/bin/uvx

USER vscode
WORKDIR /home/vscode

RUN uv tool install pebble-tool

# Add uv to PATH
ENV PATH="/home/vscode/.cargo/bin:${PATH}"

CMD [ "bash" ]
