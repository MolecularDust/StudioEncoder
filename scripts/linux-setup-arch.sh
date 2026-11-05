#!/usr/bin/env bash
# Installs what the StudioEncoder Linux build needs on Arch Linux and derivatives (CachyOS, EndeavourOS, Manjaro).
# Run it yourself: it uses sudo. Tested on CachyOS (kernel 7.3.1, OBS 33.0.1, FFmpeg 9.0.2).
#
#   bash scripts/linux-setup-arch.sh
#
# Then:  bash scripts/linux-build.sh   and   bash scripts/linux-test.sh
set -euo pipefail

echo "== build tools, OBS Studio (native package, ships the libobs headers), Qt 6, SIMDe, FFmpeg, Rust"
sudo pacman -S --needed \
	base-devel cmake ninja pkgconf git \
	obs-studio qt6-base simde ffmpeg v4l-utils \
	rustup

if ! rustup show active-toolchain >/dev/null 2>&1; then
	echo "== Rust toolchain (current user only)"
	rustup default stable
fi

echo
echo "== versions"
obs --version 2>/dev/null || true
ffmpeg -hide_banner -version | head -1
cmake --version | head -1
cargo --version
echo
echo "libobs development files:"
ls /usr/include/obs/obs-module.h /usr/include/obs/obs-frontend-api.h 2>/dev/null \
	|| echo "  NOT FOUND: the obs-studio package should provide them; see docs/HANDOFF.md, Linux section"
echo
echo "NVIDIA encoder prerequisite (only useful on RTX 50 series): the FFmpeg in use must list the high422 profile:"
if ffmpeg -hide_banner -h encoder=h264_nvenc 2>/dev/null | grep -q high422; then
	echo "  high422 present"
else
	echo "  high422 missing: the NVENC encoder will be hidden (see docs/HANDOFF.md)"
fi
