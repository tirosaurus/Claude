// Estructuras de los datos de mapa generados por tools/conv_maps.py
#pragma once
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint64_t u64;

typedef struct { s16 x, y, w, h; } Rect16;
typedef struct { const char* name; s16 x, y; } MapMarker;
typedef struct { Rect16 r; const char* to; const char* spawn; const char* requires; const char* blocked; } MapExit;
typedef struct { Rect16 r; const char* id; } MapTrigger;
typedef struct { s16 x, y, r; u8 cr, cg, cb, energy, flicker; } MapLight;
typedef struct {
	s16 x, y;
	const char* id;          // 0 si no es interactuable
	const char* sprite;
	Rect16 col;              // relativo a (x, y)
	u8 has_col;
	s8 patch;                // índice en patches o -1
	s8 light;                // índice en lights o -1
	u8 frames;
} MapProp;
typedef struct { const u16* gtile; const u8* gpal; const u16* ov; const char* name; } PatchState;
typedef struct {
	s16 prop;
	u8 cx, cy, cw, ch;       // rectángulo de celdas afectado
	u8 nstates;
	u8 kind;                 // 0 normal, 1 animado (fotogramas), 2 añadido (estado 0 = oculto)
	const PatchState* states;
} MapPatch;
typedef struct { u8 n; const char* const* enemies; } Formation;

typedef struct {
	const char* id;
	const char* display;
	const char* music;
	const char* battle_bg;
	u8 interior;
	u16 w, h, wc, hc;
	const u32* gtiles; u16 ngtiles; const u16* gcells; const u8* gcpal; const u16* pals;   // 12 paletas
	const u32* otiles; u16 notiles; const u16* ocell; const u16* olist;
	const Rect16* solids; u16 nsolids;
	const MapProp* props; u16 nprops;
	const MapPatch* patches; u8 npatches;
	const MapMarker* markers; u8 nmarkers;
	const MapExit* exits; u8 nexits;
	const MapTrigger* triggers; u8 ntriggers;
	const MapLight* lights; u8 nlights;
	u16 has_modulate, mod_r, mod_g, mod_b;
	u16 enc_rate; const Formation* enc; u8 nenc;
} MapData;

extern const MapData* const g_maps[];
extern const int g_nmaps;
