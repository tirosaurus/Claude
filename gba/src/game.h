// Estado compartido del juego (versión GBA de Vaelmoor).
#pragma once
#include "gba.h"
#include "../build/gen/maps.h"
#include "../build/gen/sprites.h"
#include "../build/gen/images.h"
#include "../build/gen/font.h"

// ---------------------------------------------------------------- banderas de historia
enum {
	F_WOKE_UP, F_MOTHER_TALKED, F_KAELEN_JOINED, F_YARA_JOINED, F_CLASS_CHOSEN, F_WOLF_SEEN, F_WON_WOLF,
	F_DEMO_END, F_COUNCIL, F_FEMALE,
	F_COUNT
};
extern u8 g_flags[F_COUNT];
#define FLAG(f) (g_flags[(f)])

// ---------------------------------------------------------------- grupo
enum { M_PLAYER, M_KAELEN, M_YARA, M_COUNT };
typedef struct {
	const char* name;
	s16 hp, mp, maxhp, maxmp, atk, def, mag, res, spd;
	u8 in_party;
} Member;
extern Member g_party[M_COUNT];
extern s16 g_level;
extern s32 g_xp;
extern s16 g_potions;
extern u8 g_class;        // 0 guerrero, 1 mago, 2 clérigo
extern char g_name[16];

// ---------------------------------------------------------------- entidades del mapa
#define MAX_ENT 12
typedef struct {
	u8 active, visible;
	const SpriteSheet* sheet;
	s32 x, y;           // pies, en píxeles de mundo (x16 = subpíxel)
	u8 facing;          // 0 abajo, 1 izquierda, 2 derecha, 3 arriba
	u8 walking;
	u16 anim;
	s16 tx, ty;         // destino al moverse por guion
	u8 moving;
	u8 follow;          // sigue al jugador (compañeros)
	u8 oam_slot;
	const char* id;
} Entity;
extern Entity g_ent[MAX_ENT];
#define ENT_PLAYER 0

// ---------------------------------------------------------------- mapa
extern int g_map;
extern s32 g_camx, g_camy;
void map_load(int map, const char* spawn);
void map_update(void);
void map_draw(void);
int map_solid_at(int x, int y);
int map_marker(const char* name, int* x, int* y);
int ent_spawn(const char* id, const SpriteSheet* sheet, int x, int y, int facing);
Entity* ent_find(const char* id);
void ent_remove(const char* id);
void party_followers_spawn(void);
void sprites_hide_all(void);

// ---------------------------------------------------------------- texto
void text_init(void);
void text_clear_box(void);
void text_hide(void);
void text_say(const char* speaker, const char* portrait, const char* msg);
int text_busy(void);
int text_done(void);
void text_freeze(void);
void text_update(void);
extern int g_choice_x;
void text_choice(const char* const* opts, int n);
int text_choice_result(void);    // -1 mientras se elige
void text_banner(const char* s);
void text_draw_string(int tile_x, int tile_y, int w_tiles, int h_tiles, const char* s, int color);
void text_draw_px(int x, int y, const char* s, int color);
void text_window(int tx, int ty, int tw, int th);
void text_fill(int tx, int ty, int tw, int th, int color);

// ---------------------------------------------------------------- guiones
void story_run_pending(void);
void story_on_map(int map);
void story_on_interact(const char* id);
void story_on_zone(const char* id);
int story_exit_allowed(int to);

// ---------------------------------------------------------------- combate
typedef struct { const char* name; s16 hp, atk, def, mag, res, spd, xp; const SpriteSheet* sheet; u8 parts; } EnemyDef;
void bg_image_load(const BgImage* img, int yoff);
int battle_run(const EnemyDef* foes, int count, int boss);   // 1 victoria, 0 derrota

// ---------------------------------------------------------------- escena / bucle
enum { SCENE_TITLE, SCENE_MAP, SCENE_BATTLE, SCENE_END };
extern int g_scene;
extern int g_cutscene;
void frame_wait(void);
extern void (*g_draw)(void);
void fade_out(void);
void fade_in(void);
void party_recalc(void);
