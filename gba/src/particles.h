// Partículas sencillas (luciérnagas, chispas, esporas, efectos de combate) como sprites 8x8.
#pragma once
#include "gfx.h"

typedef struct {
	int amount;
	float lifetime;
	float radius;           // emisión en círculo
	float rect_w, rect_h;   // o en rectángulo (mitades); si > 0 tiene prioridad
	float dir_x, dir_y;     // dirección inicial
	float spread;           // grados
	float vmin, vmax;
	float gx, gy;           // gravedad
	float size;             // 1..4 px
	u8 r, g, b;             // color
	u8 one_shot;
	float explosiveness;
	u8 fade_in;             // la intensidad sube y baja (luciérnagas)
} ParticleDef;

void particles_init(void);
int particles_emit(const ParticleDef* d, float x, float y, int layer_front);   // devuelve id de emisor
void particles_stop(int id);
void particles_clear(void);
void particles_update(void);
void particles_render(int camx, int camy, u32 key_base);
