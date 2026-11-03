#!/usr/bin/env bash
# Installs what the StudioEncoders Linux build needs on Ubuntu 24.04 or newer (and derivatives).
# Run it yourself: it uses sudo and adds the official OBS Studio PPA.
#
#   bash scripts/linux-setup-ubuntu.sh
set -euo pipefail

echo "== OBS Studio from the official PPA (native package; the Flatpak cannot load this plugin as is)"
sudo add-apt-repository -y ppa:obsproject/obs-studio
sudo apt-get update

echo "== build tools, OBS, Qt 6, FFmpeg development files, FFmpeg command line (for tests)"
sudo apt-get install -y \
	build-essential cmake ninja-build pkg-config git curl file \
	obs-studio qt6-base-dev libsimde-dev \
	libavcodec-dev libavutil-dev libavformat-dev libswscale-dev ffmpeg

if ! command -v cargo >/dev/null 2>&1; then
	echo "== Rust (rustup, current user only)"
	curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal
	echo "Run: source \"$HOME/.cargo/env\"   (or open a new terminal) before building"
fi

echo
echo "== versions"
obs --version 2>/dev/null || true
ffmpeg -hide_banner -version | head -1
cmake --version | head -1
command -v cargo >/dev/null 2>&1 && cargo --version || echo "cargo: open a new terminal first"
echo
echo "libobs development files:"
ls /usr/include/obs/obs-module.h 2>/dev/null || echo "  NOT FOUND: see docs/HANDOFF.md, Linux section (build OBS from source)"
ls /usr/include/obs/obs-frontend-api.h /usr/include/obs/frontend/api/obs-frontend-api.h 2>/dev/null || true
