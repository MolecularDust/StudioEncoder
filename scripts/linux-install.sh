#!/usr/bin/env bash
# Installs the Linux build into the current user's OBS plugin folder (no sudo):
#   ~/.config/obs-studio/plugins/obs-studioencoder/bin/64bit/obs-studioencoder.so
#   ~/.config/obs-studio/plugins/obs-studioencoder/data/  (locale, manifest.json)
# and removes copies under the earlier names (obs-prores, obs-studioencoders) so OBS never loads two. Close OBS first.
#
#   bash scripts/linux-install.sh
set -euo pipefail
cd "$(dirname "$0")/.."

name=obs-studioencoder
base="${XDG_CONFIG_HOME:-$HOME/.config}/obs-studio/plugins"
dest="$base/$name"

if pgrep -x obs >/dev/null 2>&1; then
	echo "OBS is running: close it first." >&2
	exit 1
fi

so=$(find build_x86_64 -maxdepth 2 -name "$name.so*" -type f | head -1)
[ -n "$so" ] || { echo "No $name.so in build_x86_64: run scripts/linux-build.sh first." >&2; exit 1; }

rm -rf "$base/obs-prores" "$base/obs-studioencoders"
rm -rf "$dest"
mkdir -p "$dest/bin/64bit" "$dest/data"
cp -L "$so" "$dest/bin/64bit/$name.so"
cp -r data/. "$dest/data/"
cp build_x86_64/manifest.json "$dest/data/manifest.json"

echo "Installed to $dest"
find "$dest" -type f | sed "s|$dest|  |"
