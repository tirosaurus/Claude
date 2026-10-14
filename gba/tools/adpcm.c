// Codificador IMA ADPCM de 4 bits (s16 mono crudo -> nibbles, nibble bajo primero).
// Uso: adpcm in.raw out.bin  -> imprime "pred index" iniciales
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
static const int idx_tab[16] = {-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};
static const int step_tab[89] = {7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,
 130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,
 1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,
 13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
int main(int argc, char** argv) {
	FILE* f = fopen(argv[1], "rb"); fseek(f, 0, SEEK_END); long n = ftell(f) / 2; fseek(f, 0, SEEK_SET);
	int16_t* s = malloc(n * 2); fread(s, 2, n, f); fclose(f);
	uint8_t* out = calloc((n + 1) / 2, 1);
	int pred = s[0], index = 20;
	printf("%d %d\n", pred, index);
	for (long i = 0; i < n; ++i) {
		int step = step_tab[index];
		int diff = s[i] - pred, code = 0;
		if (diff < 0) { code = 8; diff = -diff; }
		int vd = step >> 3;
		if (diff >= step) { code |= 4; diff -= step; vd += step; }
		if (diff >= step >> 1) { code |= 2; diff -= step >> 1; vd += step >> 1; }
		if (diff >= step >> 2) { code |= 1; vd += step >> 2; }
		pred += (code & 8) ? -vd : vd;
		if (pred > 32767) pred = 32767; if (pred < -32768) pred = -32768;
		index += idx_tab[code]; if (index < 0) index = 0; if (index > 88) index = 88;
		out[i >> 1] |= (i & 1) ? code << 4 : code;
	}
	f = fopen(argv[2], "wb"); fwrite(out, 1, (n + 1) / 2, f); fclose(f);
	return 0;
}
