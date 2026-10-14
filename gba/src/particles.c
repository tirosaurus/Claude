#include "particles.h"

#define MAXP 72
#define MAXE 8
typedef struct { s32 x, y, vx, vy; u16 life, age; u8 em; u8 size; } Part;
typedef struct { ParticleDef d; s32 x, y; u8 used, front; float acc; int emitted; int pal; } Emitter;
static Part parts[MAXP];
static Emitter em[MAXE];
static int tile_base = -1;
static u16 em_pal[MAXE][16];

static void make_tiles(void) {
	if (tile_base >= 0) return;
	tile_base = objt_alloc(4);
	if (tile_base < 0) return;
	// 4 tamaños: 1,2,3,4 px (núcleo 1, borde 2)
	for (int s = 0; s < 4; ++s) {
		volatile u32* t = objt_ptr(tile_base + s);
		for (int r = 0; r < 8; ++r) {
			u32 row = 0;
			for (int c = 0; c < 8; ++c) {
				int sz = s + 1;
				int in = r < sz && c < sz;
				int core = in && (sz <= 2 || (r > 0 && c > 0 && r < sz - 1 && c < sz - 1));
				if (in) row |= (u32)(core ? 1 : 2) << (c * 4);
			}
			t[r] = row;
		}
	}
}

void particles_init(void) {
	memset(parts, 0, sizeof parts);
	memset(em, 0, sizeof em);
	tile_base = -1;
}
void particles_clear(void) {
	for (int i = 0; i < MAXE; ++i) if (em[i].used && em[i].pal >= 0) objpal_put(em[i].pal);
	memset(parts, 0, sizeof parts);
	memset(em, 0, sizeof em);
}

int particles_emit(const ParticleDef* d, float x, float y, int front) {
	make_tiles();
	for (int i = 0; i < MAXE; ++i) {
		if (em[i].used) continue;
		Emitter* e = &em[i];
		e->d = *d;
		e->x = (s32)(x * 256); e->y = (s32)(y * 256);
		e->used = 1;
		e->front = front;
		e->acc = 0;
		e->emitted = 0;
		u16* p = em_pal[i];
		memset(p, 0, 32);
		p[1] = (d->r >> 3) | ((d->g >> 3) << 5) | ((d->b >> 3) << 10);
		p[2] = ((d->r * 3 / 4) >> 3) | (((d->g * 3 / 4) >> 3) << 5) | (((d->b * 3 / 4) >> 3) << 10);
		e->pal = objpal_get(p, 256, 256, 256, 0);
		if (d->one_shot && d->explosiveness > 0.5f) {
			for (int k = 0; k < d->amount; ++k) {
				extern void spawn_part(int ei);
				spawn_part(i);
			}
			e->emitted = d->amount;
		}
		return i;
	}
	return -1;
}

void particles_set_pos(int id, float x, float y) {
	if (id < 0 || id >= MAXE || !em[id].used) return;
	em[id].x = (s32)(x * 256); em[id].y = (s32)(y * 256);
}

void particles_stop(int id) {
	if (id < 0 || id >= MAXE || !em[id].used) return;
	em[id].d.one_shot = 1;
	em[id].emitted = em[id].d.amount;
}

void spawn_part(int ei) {
	Emitter* e = &em[ei];
	for (int i = 0; i < MAXP; ++i) {
		if (parts[i].life) continue;
		Part* p = &parts[i];
		const ParticleDef* d = &e->d;
		float ox, oy;
		if (d->rect_w > 0) { ox = randf_range(-d->rect_w, d->rect_w); oy = randf_range(-d->rect_h, d->rect_h); }
		else { float a = randf() * 6.2831853f, r = d->radius * randf(); ox = r * sinf_fast(a + 1.5707963f); oy = r * sinf_fast(a); }
		float ang = randf_range(-d->spread, d->spread) * (3.14159265f / 180.0f);
		float dx = d->dir_x, dy = d->dir_y;
		float c = sinf_fast(ang + 1.5707963f), s = sinf_fast(ang);
		float vx = dx * c - dy * s, vy = dx * s + dy * c;
		float len = vx * vx + vy * vy;
		if (len > 0.0001f) { float inv = 1.0f / (float)isqrt((u32)(len * 65536)) * 256.0f; vx *= inv; vy *= inv; }
		float v = randf_range(d->vmin, d->vmax);
		p->x = e->x + (s32)(ox * 256);
		p->y = e->y + (s32)(oy * 256);
		p->vx = (s32)(vx * v * 256 / 60);
		p->vy = (s32)(vy * v * 256 / 60);
		p->life = (u16)(d->lifetime * 60);
		if (!p->life) p->life = 1;
		p->age = 0;
		p->em = ei;
		p->size = (u8)CLAMP((int)(d->size + randf_range(-0.5f, 0.5f)), 1, 4);
		return;
	}
}

void particles_update(void) {
	for (int i = 0; i < MAXE; ++i) {
		Emitter* e = &em[i];
		if (!e->used) continue;
		if (!e->d.one_shot || e->emitted < e->d.amount) {
			e->acc += e->d.amount / (e->d.lifetime * 60.0f);
			while (e->acc >= 1.0f) {
				e->acc -= 1.0f;
				spawn_part(i);
				if (e->d.one_shot && ++e->emitted >= e->d.amount) break;
			}
		}
	}
	int alive[MAXE] = {0};
	for (int i = 0; i < MAXP; ++i) {
		Part* p = &parts[i];
		if (!p->life) continue;
		Emitter* e = &em[p->em];
		p->vx += (s32)(e->d.gx * 256 / 3600);
		p->vy += (s32)(e->d.gy * 256 / 3600);
		p->x += p->vx;
		p->y += p->vy;
		if (++p->age >= p->life) p->life = 0;
		else alive[p->em]++;
	}
	for (int i = 0; i < MAXE; ++i)
		if (em[i].used && em[i].d.one_shot && em[i].emitted >= em[i].d.amount && !alive[i]) {
			objpal_put(em[i].pal);
			em[i].used = 0;
		}
}

void particles_render(int camx, int camy, u32 key_base) {
	if (tile_base < 0) return;
	for (int i = 0; i < MAXP; ++i) {
		Part* p = &parts[i];
		if (!p->life) continue;
		Emitter* e = &em[p->em];
		if (!e->used) { p->life = 0; continue; }
		int t = p->age * 256 / p->life;
		int sz = p->size;
		// desvanecer: encoger al final (y crecer al principio si fade_in)
		if (t > 170) sz = sz * (256 - t) / 86 + 1;
		if (e->d.fade_in && t < 86) sz = sz * t / 86 + 1;
		if (sz > 4) sz = 4;
		if (e->d.fade_in && t < 30) continue;
		int x = (p->x >> 8) - camx, y = (p->y >> 8) - camy;
		if (x < -8 || x > 240 || y < -8 || y > 160) continue;
		oam_push(key_base + i, obj_a0(y, OBJ_SQUARE, 0), obj_a1(x, 0, 0), obj_a2(tile_base + sz - 1, e->front ? 1 : 2, e->pal));
	}
}
