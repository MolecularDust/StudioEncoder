/* prores-aw-test <in.yuv> <width> <height> <frames> <profile 0-3> <round8 0-7> <out.bin>
 * Encodes planar yuv422p10le frames with src/prores-aw.c and writes the ProRes frames back to back
 * (each starts with its 4-byte size), for comparison with FFmpeg's prores_aw output. */
#include "../src/prores-aw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
	if (argc == 3 && strcmp(argv[1], "selftest") == 0) {
		const long n = atol(argv[2]);
		const long bad = pa_selftest_simd(n);
		printf("AVX2 vs C DCT: %ld blocks, %ld mismatches\n", n, bad);
		return bad == 0 ? 0 : 1;
	}
	if (argc != 8) {
		fprintf(stderr, "usage: %s in.yuv w h frames profile round8 out.bin\n", argv[0]);
		return 2;
	}
	const int w = atoi(argv[2]), h = atoi(argv[3]), n = atoi(argv[4]), profile = atoi(argv[5]),
		  round8 = atoi(argv[6]);
	FILE *in = fopen(argv[1], "rb"), *out = fopen(argv[7], "wb");
	if (!in || !out)
		return 1;
	const size_t ny = (size_t)w * h, nc = (size_t)(w / 2) * h;
	uint16_t *frame = malloc((ny + 2 * nc) * sizeof(uint16_t));
	const size_t cap = pa_max_frame_size(w, h);
	uint8_t *pkt = malloc(cap);
	struct pa_ctx *ctx = pa_create(w, h, profile, round8, "apl0", 2, 2, 2);
	if (getenv("PA_NO_SIMD"))
		pa_set_simd(0);
	fprintf(stderr, "DCT: %s\n", pa_simd_active() ? "AVX2" : "C");
	if (!frame || !pkt || !ctx)
		return 1;
	double secs = 0;
	size_t total = 0;
	int done = 0;
	for (; done < n; done++) {
		if (fread(frame, sizeof(uint16_t), ny + 2 * nc, in) != ny + 2 * nc)
			break;
		const clock_t t0 = clock();
		const int64_t sz = pa_encode(ctx, frame, (size_t)w, frame + ny, frame + ny + nc, (size_t)(w / 2), pkt, cap);
		secs += (double)(clock() - t0) / CLOCKS_PER_SEC;
		if (sz < 0) {
			fprintf(stderr, "encode failed at frame %d\n", done);
			return 1;
		}
		fwrite(pkt, 1, (size_t)sz, out);
		total += (size_t)sz;
	}
	printf("frames=%d avg_kb=%zu ms_per_frame=%.1f\n", done, total / (done ? done : 1) / 1024, secs * 1000 / (done ? done : 1));
	pa_destroy(ctx);
	fclose(in);
	fclose(out);
	return 0;
}
