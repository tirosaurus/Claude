// Composición de sprites del jugador y del grupo (port de Appearance.gd): capas del creador
// teñidas con los colores elegidos, armadura, casco, escudo y arma visibles.
#pragma once
#include "state.h"
#include "gfxdata.h"

typedef struct {
	u32* tiles;          // mapa: 32 frames x 12 tiles (3x4, frame 20x30 en (2,2)); combate: 9 x 16; retrato: 36
	u16 pal[16];
	u32 key;
	u8 valid, kind;
} Composed;

void appearance_dirty(void);
const Composed* member_sheet(int m);          // hoja de mapa (8x4 frames)
const Composed* member_battle_sheet(int m);   // 9 poses 32x32
const Composed* member_portrait(int m);       // 48x48
const Composed* preview_sheet(const Appearance* a);
const Composed* preview_portrait(const Appearance* a);
const Portrait* portrait_by_name(const char* name);   // retratos fijos de PNJ
const Sheet* char_sheet(const char* id);              // hoja fija de PNJ (assets/chars)
const Sheet* battle_sheet_static(const char* id);
// sube el frame (0..31) de una hoja compuesta de mapa a la VRAM de OBJ (16 tiles)
void composed_upload_frame(const Composed* c, int frame, int tile);
