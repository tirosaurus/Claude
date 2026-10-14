#include "audio.h"

#define SPF 304                     // muestras por fotograma (18157 Hz)
#define N_SFX 8

u8 g_music_level = 8, g_sfx_level = 8, g_text_speed = 1, g_battle_speed = 1;

static s8 mixbuf[2][SPF] __attribute__((aligned(4)));
static u8 mixcur;

typedef struct {
	const MusicTrack* t;
	u32 pos;
	s32 pred;
	s32 index;
	s32 vol, target, step;          // Q16
	u8 active;
} MusicVoice;
typedef struct {
	const s8* data;
	u32 n;
	u32 pos;                        // Q12
	u32 step;                       // Q12
	s32 vol;                        // Q8
	u8 active;
} SfxVoice;

static MusicVoice mv[2];
static SfxVoice sv[N_SFX];
static const char* cur_name;
static s32 music_master = 0;        // Q8

static s8 idx_tab[16] __attribute__((section(".iwram.tab"))) = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
static u16 step_tab[89] __attribute__((section(".iwram.tab"))) = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73,
	80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876,
	963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
	7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

static s32 acc[SPF];
IWRAM_CODE static void mix(s8* out) {
	for (int i = 0; i < SPF; ++i) acc[i] = 0;
	for (int v = 0; v < 2; ++v) {
		MusicVoice* m = &mv[v];
		if (!m->active) continue;
		const MusicTrack* t = m->t;
		s32 pred = m->pred, index = m->index;
		u32 pos = m->pos;
		s32 vol = m->vol;
		s32 gain = (vol >> 8) * music_master >> 8;
		const u8* data = t->data;
		u32 nsamp = t->nsamples;
		int i = 0;
		while (i < SPF) {
			if (pos >= nsamp) {
				if (!t->loop) { m->active = 0; break; }
				pos = 0; pred = t->pred0; index = t->idx0;
			}
			// tramo sin fin de pista ni cambio de volumen: bucle compacto
			int n = SPF - i;
			if ((u32)n > nsamp - pos) n = nsamp - pos;
			if (m->step && n > 16) n = 16;
			for (int k = 0; k < n; ++k, ++i, ++pos) {
				int code = (data[pos >> 1] >> ((pos & 1) << 2)) & 15;
				int step = step_tab[index];
				int vd = step >> 3;
				if (code & 4) vd += step;
				if (code & 2) vd += step >> 1;
				if (code & 1) vd += step >> 2;
				pred += (code & 8) ? -vd : vd;
				if (pred > 32767) pred = 32767; else if (pred < -32768) pred = -32768;
				index += idx_tab[code];
				if (index < 0) index = 0; else if (index > 88) index = 88;
				acc[i] += (pred * gain) >> 16;
			}
			if (m->step) {
				vol += m->step * n;
				if ((m->step > 0 && vol >= m->target) || (m->step < 0 && vol <= m->target)) {
					vol = m->target; m->step = 0;
					if (vol <= 0) { m->active = 0; break; }
				}
				gain = (vol >> 8) * music_master >> 8;
			}
		}
		m->pred = pred; m->index = index; m->pos = pos; m->vol = vol;
	}
	for (int v = 0; v < N_SFX; ++v) {
		SfxVoice* s = &sv[v];
		if (!s->active) continue;
		u32 pos = s->pos, step = s->step, end = s->n << 12;
		s32 vol = s->vol;
		const s8* d = s->data;
		for (int i = 0; i < SPF; ++i) {
			if (pos >= end) { s->active = 0; break; }
			acc[i] += (d[pos >> 12] * vol) >> 8;
			pos += step;
		}
		s->pos = pos;
	}
	for (int i = 0; i < SPF; ++i) {
		s32 a = acc[i];
		out[i] = a > 127 ? 127 : (a < -128 ? -128 : a);
	}
}

volatile u32 g_vbl_count;
IWRAM_CODE void isr_vblank(void) {
	g_vbl_count++;
	REG_DMA1CNT = 0;
	REG_DMA1SAD = (u32)mixbuf[mixcur];
	REG_DMA1CNT = 0xB6400000;     // activar, repetir, 32 bits, inicio por FIFO, destino fijo
	mixcur ^= 1;
	mix(mixbuf[mixcur]);
}

void audio_init(void) {
	REG_SOUNDCNT_X = 0x80;
	REG_SOUNDCNT_H = 0x0B04 | 0x0002;   // DS-A al 100 %, ambos altavoces, timer 0, reset FIFO; PSG al 100 %
	REG_SOUNDCNT_L = 0;
	REG_DMA1DAD = (u32)&REG_FIFO_A;
	REG_TM0CNT_H = 0;
	REG_TM0CNT_L = 65536 - 924;
	REG_TM0CNT_H = 0x80;
	audio_apply_settings();
}

// ------------------------------------------------------------------ dB
static s32 db_to_q8(float db) {
	if (db <= -60.0f) return 0;
	// 10^(db/20) = 2^(db * 0.166096)
	float e = db * 0.166096f;
	int ip = (int)e;
	if (e < 0 && ip != e) ip -= 1;
	float fr = e - ip;               // 0..1
	float p = 1.0f + fr * (0.6931472f + fr * (0.2402265f + fr * 0.0555041f));
	float v = p * 256.0f;
	if (ip >= 0) v *= (float)(1 << ip);
	else v /= (float)(1 << (-ip));
	return (s32)v;
}

static s32 level_q8(int l) { return l <= 0 ? 0 : l * 256 / 10; }

void audio_apply_settings(void) {
	// music_volume_db = -6 + level_db(music_level)
	music_master = (db_to_q8(-6.0f) * level_q8(g_music_level)) >> 8;
}

float text_speed_mult(void) { static const float t[3] = {0.6f, 1.0f, 1.8f}; return t[g_text_speed > 2 ? 2 : g_text_speed]; }
float battle_speed_mult(void) { static const float t[3] = {0.75f, 1.0f, 1.4f}; return t[g_battle_speed > 2 ? 2 : g_battle_speed]; }

static const MusicTrack* find_track(const char* name) {
	for (int i = 0; i < g_nmusic; ++i)
		if (!strcmp(g_music[i].name, name)) return &g_music[i];
	return 0;
}

const char* music_current(void) { return cur_name ? cur_name : ""; }

void music_play(const char* track, float fade) {
	if (!track || !track[0]) return;
	if (cur_name && !strcmp(cur_name, track)) return;
	const MusicTrack* t = find_track(track);
	if (!t) return;
	cur_name = t->name;
	int frames = (int)(fade * FPS);
	if (frames < 1) frames = 1;
	u32 samples = frames * SPF;
	REG_IME = 0;
	// la voz que suena se desvanece; la nueva entra en la otra
	int nv = mv[0].active && mv[0].target > 0 ? 1 : 0;
	MusicVoice* old = &mv[nv ^ 1];
	if (old->active) {
		old->target = 0;
		old->step = -(old->vol / (s32)samples) - 1;
	}
	MusicVoice* m = &mv[nv];
	m->t = t; m->pos = 0; m->pred = t->pred0; m->index = t->idx0;
	m->vol = 655;                       // ~ -40 dB
	m->target = 65536;
	m->step = (65536 - m->vol) / (s32)samples + 1;
	m->active = 1;
	REG_IME = 1;
}

void music_stop(float fade) {
	cur_name = 0;
	int frames = (int)(fade * FPS);
	if (frames < 1) frames = 1;
	u32 samples = frames * SPF;
	REG_IME = 0;
	for (int v = 0; v < 2; ++v)
		if (mv[v].active) { mv[v].target = 0; mv[v].step = -(mv[v].vol / (s32)samples) - 1; }
	REG_IME = 1;
}

void sfx(const char* name, float db, float pitch) {
	const SfxSample* s = 0;
	for (int i = 0; i < g_nsfx; ++i)
		if (!strcmp(g_sfx[i].name, name)) { s = &g_sfx[i]; break; }
	if (!s) return;
	s32 vol = (db_to_q8(db) * level_q8(g_sfx_level)) >> 8;
	if (vol <= 0) return;
	int slot = -1;
	for (int i = 0; i < N_SFX; ++i) if (!sv[i].active) { slot = i; break; }
	if (slot < 0) slot = 0;
	REG_IME = 0;
	SfxVoice* v = &sv[slot];
	v->data = s->data; v->n = s->n; v->pos = 0;
	v->step = (u32)(pitch * 4096.0f);
	if (!v->step) v->step = 4096;
	v->vol = vol;
	v->active = 1;
	REG_IME = 1;
}
