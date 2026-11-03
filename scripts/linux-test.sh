#!/usr/bin/env bash
# Exactness and speed checks for AW+ on Linux, the same ones run on Windows (see docs/HANDOFF.md).
# Needs build_x86_64/prores-aw-test (scripts/linux-build.sh) and the ffmpeg command line.
# Uses a generated test picture (no camera clip needed); a 4K clip from the Windows drive can be passed instead:
#
#   bash scripts/linux-test.sh                      # generated 4K test pattern
#   bash scripts/linux-test.sh /path/to/clip.mov    # first 5 frames of a real 4K 4:2:2 clip
set -euo pipefail
cd "$(dirname "$0")/.."

T="$PWD/build_x86_64/prores-aw-test"
[ -x "$T" ] || { echo "Build first: bash scripts/linux-build.sh" >&2; exit 1; }
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT
fails=0
eq() { if cmp -s "$1" "$2"; then echo "IDENTICAL"; else echo "DIFFERENT"; fails=$((fails + 1)); fi; }

echo "== 1. AVX2 DCT against the plain C DCT, 4 million blocks"
"$T" selftest 4000000 || fails=$((fails + 1))

echo "== 2. source frames"
if [ $# -ge 1 ]; then
	ffmpeg -hide_banner -v error -y -i "$1" -frames:v 5 -vf scale=3840:2160 -pix_fmt yuv422p10le -f rawvideo "$W/src.yuv"
else
	ffmpeg -hide_banner -v error -y -f lavfi -i "testsrc2=s=3840x2160:r=30,noise=alls=12:allf=t" -frames:v 5 \
		-pix_fmt yuv422p10le -f rawvideo "$W/src.yuv"
fi
ffmpeg -hide_banner -v error -y -f rawvideo -pix_fmt yuv422p10le -s 3840x2160 -i "$W/src.yuv" \
	-vf scale=1918:1082 -pix_fmt yuv422p10le -f rawvideo "$W/odd.yuv"
ffmpeg -hide_banner -version | head -1

echo "== 3. rounding off: byte-identical to this system's FFmpeg prores_aw (AVX2 and plain C)"
for s in 3840x2160 1918x1082; do
	in="$W/src.yuv"; [ "$s" = 1918x1082 ] && in="$W/odd.yuv"
	w=${s%x*}; h=${s#*x}
	for p in 0 1 2 3; do
		ffmpeg -hide_banner -v error -y -f rawvideo -pix_fmt yuv422p10le -s "$s" -r 30 -i "$in" \
			-c:v prores_aw -profile:v $p -vendor apl0 "$W/ff.mov"
		ffmpeg -hide_banner -v error -y -i "$W/ff.mov" -map 0:v -c copy -f data "$W/ff.bin"
		"$T" "$in" "$w" "$h" 5 $p 0 "$W/a.bin" 2>/dev/null >/dev/null
		PA_NO_SIMD=1 "$T" "$in" "$w" "$h" 5 $p 0 "$W/c.bin" 2>/dev/null >/dev/null
		echo "  $s profile $p: AVX2 $(eq "$W/a.bin" "$W/ff.bin"), plain C $(eq "$W/c.bin" "$W/ff.bin")"
	done
done

echo "== 4. AW+ rounding (as shipped): AVX2 and plain C give the same bytes"
def=(4 3 3 2)
for p in 0 1 2 3; do
	"$T" "$W/src.yuv" 3840 2160 5 $p ${def[$p]} "$W/a.bin" 2>/dev/null >/dev/null
	PA_NO_SIMD=1 "$T" "$W/src.yuv" 3840 2160 5 $p ${def[$p]} "$W/c.bin" 2>/dev/null >/dev/null
	echo "  profile $p: $(eq "$W/a.bin" "$W/c.bin")"
done

echo "== 5. speed, one thread, 4K (ms per frame)"
names=(Proxy LT 422 HQ)
for p in 0 1 2 3; do
	a=$("$T" "$W/src.yuv" 3840 2160 5 $p ${def[$p]} "$W/a.bin" 2>/dev/null | grep -o 'ms_per_frame=[0-9.]*' | cut -d= -f2)
	c=$(PA_NO_SIMD=1 "$T" "$W/src.yuv" 3840 2160 5 $p ${def[$p]} "$W/c.bin" 2>/dev/null | grep -o 'ms_per_frame=[0-9.]*' | cut -d= -f2)
	echo "  ${names[$p]}: AVX2 $a, plain C $c"
done

echo
if [ "$fails" -eq 0 ]; then echo "ALL CHECKS PASSED"; else echo "$fails CHECK(S) FAILED"; exit 1; fi
