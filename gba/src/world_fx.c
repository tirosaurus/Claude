// Partículas ambientales de cada mapa (World._build_particles) y polvo de escena
#include "world.h"
#include "particles.h"

static int amb = -1;
static float density;

void particles_setup(void) {
	particles_init();
	ParticleDef d;
	memset(&d, 0, sizeof d);
	const char* id = W.m->id;
	d.size = 2;
	if (!strcmp(id, "forest") || !strcmp(id, "elf_camp")) {
		d.amount = 70; d.lifetime = 6; d.vmin = 3; d.vmax = 10; d.dir_x = 0.3f; d.dir_y = -1; d.spread = 180;
		d.r = 230; d.g = 255; d.b = 153; d.fade_in = 1;
	} else if (!strcmp(id, "forest_deep") || !strcmp(id, "heart")) {
		d.amount = 80; d.lifetime = 6; d.vmin = 2; d.vmax = 8; d.dir_x = 0; d.dir_y = -1; d.spread = 180;
		d.r = 230; d.g = 128; d.b = 255; d.fade_in = 1;
	} else if (!strcmp(id, "village_night")) {
		d.amount = 60; d.lifetime = 4; d.vmin = 10; d.vmax = 30; d.dir_x = 0.2f; d.dir_y = -1; d.spread = 30;
		d.r = 255; d.g = 179; d.b = 77;
	} else if (!strcmp(id, "village") || !strcmp(id, "village_dawn")) {
		d.amount = 24; d.lifetime = 8; d.vmin = 6; d.vmax = 14; d.dir_x = 1; d.dir_y = 0.4f; d.spread = 20; d.size = 1;
		if (!strcmp(id, "village")) { d.r = 255; d.g = 245; d.b = 217; } else { d.r = 179; d.g = 179; d.b = 184; }
	} else {
		d.amount = 18; d.lifetime = 7; d.vmin = 1; d.vmax = 4; d.dir_x = 0.2f; d.dir_y = 1; d.spread = 180; d.size = 1;
		d.r = 200; d.g = 190; d.b = 160;
	}
	// misma densidad que en Godot, pero solo alrededor de la cámara
	density = (float)(240 * 160) / (float)(W.m->w * W.m->h);
	if (density > 1) density = 1;
	d.amount = (int)(d.amount * density * 1.4f + 0.5f);
	if (d.amount < 2) d.amount = 2;
	d.rect_w = 140; d.rect_h = 100;
	amb = particles_emit(&d, PX(W.cam_x), PX(W.cam_y), 1);
}

extern int world_cam_left(void), world_cam_top(void);
void particles_tick(void) {
	extern void particles_set_pos(int id, float x, float y);
	if (amb >= 0) particles_set_pos(amb, PX(W.cam_x), PX(W.cam_y));
	particles_update();
}
void particles_draw(int camx, int camy) { particles_render(camx, camy, 0x10000000); }

void dust(int x, int y) {
	ParticleDef d;
	memset(&d, 0, sizeof d);
	d.amount = 30; d.one_shot = 1; d.explosiveness = 0.9f; d.lifetime = 0.9f; d.radius = 10;
	d.dir_x = 0; d.dir_y = -1; d.spread = 80; d.gy = 40; d.vmin = 20; d.vmax = 60; d.size = 3;
	d.r = 179; d.g = 166; d.b = 153;
	particles_emit(&d, x, y, 1);
}
