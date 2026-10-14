// Estructuras de los gráficos generados por tools/conv_gfx.py
#pragma once
#include "mapdata.h"

typedef struct { const u32* tiles; const u16* pal; u16 nframes; u8 w, h; } Sheet;          // frames w x h (1D)
typedef struct { s8 x, y; u8 shape, size; u16 tile; } ObjPiece;                             // relativo al ancla
typedef struct {
	const u32* tiles; const u16* pal;
	u8 nframes, npieces; u16 frame_tiles;
	s16 w, h;
	const ObjPiece* pieces;
} MetaSprite;
typedef struct { u8 wc, hc; u16 ntiles; const u32* tiles; const u16* cells; const u16* pals; u8 npal; } BgImage;
typedef struct { const u32* tiles; const u16* pal; } Portrait;                              // 48x48, 36 tiles
typedef struct { const char* name; u16 w, h; u8 ncol; const u8* rgb; const u8* rle; } Layer;
typedef struct { const u8* px; u8 w, h, margin; } NineSrc;
typedef struct { const char* name; u8 pal; } IconDef;
typedef struct { const char* key; const MetaSprite* ms; } EnemyGfx;
typedef struct { const char* name; const MetaSprite* ms; } SceneSprite;
typedef struct { const u16* codes; const u8* adv; const u16* rows; u16 count; u8 height; } Font;
