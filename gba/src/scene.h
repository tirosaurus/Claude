// Cambio de escena (título, creación, intro, mundo, combate, final)
#pragma once
#include "core.h"

enum { SC_TITLE, SC_CREATION, SC_INTRO, SC_WORLD, SC_BATTLE, SC_ENDING, SC_COUNT };
extern int g_scene_next;
extern int g_scene_cur;
void scene_request(int sc);           // la escena activa termina cuando le conviene
int scene_requested(void);
void block_forever(void);             // para guiones que provocan un cambio de escena

// encuentro pendiente (GameState.pending_encounter) y retorno del combate
typedef struct {
	char id[24];
	char enemies[6][20];
	u8 n;
	char bg[24];
	char music[16];
	u8 boss, can_flee, vael, weaken_aleixolo;
	char ret_map[20];
	u8 ret_has_pos;
	s16 ret_x, ret_y;
	char ret_spawn[24];
} Encounter;
extern Encounter g_enc;
