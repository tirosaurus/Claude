// Mundo (port de World.gd, Player.gd y NPC.gd): mapa, cámara, entidades, colisiones,
// caminos, interacción, salidas, zonas, encuentros y escenas.
#pragma once
#include "gfx.h"
#include "state.h"
#include "appearance.h"

#define FX 8                          // posiciones en punto fijo x256
#define TO_FX(v) ((s32)((v) * 256))
#define PX(v) ((v) >> FX)

enum { ENT_NONE, ENT_PLAYER, ENT_NPC, ENT_SPRITE };
enum { MODE_IDLE, MODE_WANDER, MODE_FOLLOW };
#define MAX_ENT 24
#define MAX_PATH 48

typedef struct Ent {
	u8 used, kind, mode, facing, busy, visible;
	char id[20];
	s32 x, y;                         // pies (x256)
	// apariencia
	s8 member;                        // >=0: hoja compuesta del grupo
	const Sheet* sheet;               // hoja fija (PNJ)
	const MetaSprite* ms;             // sprite de escena (monstruos)
	u8 ms_frame, flip, ms_auto;
	s32 anim_t;                       // segundos x256
	u8 breath_off, frame;
	s16 hop_y, off_x;                 // desplazamiento del sprite
	u8 tr, tg, tb;                    // modulate (255 = blanco)
	u8 alpha;                         // 0..16
	s16 rot;                          // 0..1024 (una vuelta)
	u16 scale;                        // x256 (sprites de escena)
	// seguir
	u8 follow_gap;
	s32 follow_side;                  // x256
	// pasear
	s32 home_x, home_y, wtx, wty, wradius;
	s32 wwait;                        // fotogramas
	// camino
	s16 px[MAX_PATH], py[MAX_PATH];
	u8 pn, pi;
	s32 speed;                        // px/s x256
	u8 moving, walk_anim;
	// tween de posición (move_node)
	s32 tw_fx, tw_fy, tw_tx, tw_ty;
	u16 tw_t, tw_n;
	// vídeo
	s16 tile, ntiles, pal;
	s16 up_frame;
	const void* up_src;
	u8 pal_key_dirty;
} Ent;

extern Ent g_ent[MAX_ENT];
#define PLAYER (&g_ent[0])

typedef struct { const char* who; const char* text; } Line;
typedef struct { Line l[24]; int n; } LineBuf;

// ganchos de la historia de cada mapa (NULL = comportamiento por defecto de StoryBase)
typedef struct {
	const char* map;
	const char* (*music_override)(void);
	void (*on_map_ready)(void);
	void (*on_trigger)(const char* id);
	void (*on_interact)(const char* id);          // sustituye a StoryBase.on_interact
	int (*interact_lines)(const char* id, LineBuf* out);
	void (*on_npc)(const char* id);
	int (*npc_lines)(const char* id, LineBuf* out);
	int (*encounters_disabled)(void);
	const char* (*exit_requirement)(const char* to, const char* def);
	const char* (*exit_blocked_message)(const char* to, const char* def);
	int (*before_exit)(const char* to);
	int (*is_interactable)(const char* id);
	void (*open_chest)(const char* id);
} StoryDef;

typedef struct {
	const MapData* m;
	const StoryDef* story;
	u8 cutscene;
	u8 pstate[64];                    // estado de cada parche
	u8 prop_removed[256];
	s32 cam_x, cam_y;                 // centro de la cámara (x256)
	u8 cam_focus;
	s32 focus_x, focus_y;
	s16 shake_x, shake_y;
	u8 dark;
	int tint_r, tint_g, tint_b;       // modulate del mapa (x256)
} World;
extern World W;

// ------------------------------------------------------------------ escena del mundo
void world_scene(void* arg);          // tarea principal mientras se juega en el mapa
const MapData* map_by_id(const char* id);
const char* resolve_target(const char* to);
void map_modulate(int* r, int* g, int* b);

// ------------------------------------------------------------------ entidades
Ent* spawn_npc(const char* id, int x, int y, int face, int mode, const char* sheet_name);
Ent* npc(const char* id);
void remove_npc(const char* id);
Ent* spawn_follower(int m);
void make_follower(int m);
Ent* monster(const char* scene_sprite, int x, int y, int flip);   // sprite de escena animado
Ent* scene_sprite(const char* scene_sprite, int x, int y, int flip, int animate);
void ent_remove(Ent* e);
void ent_face(Ent* e, int dir);
void ent_face_towards(Ent* e, int x, int y);
void walk_to(Ent* e, int x, int y, float speed, int direct);      // bloquea
void walk_to_async(Ent* e, int x, int y, float speed, int direct);
int ent_walking(Ent* e);
void ent_hop(Ent* e);                                               // bloquea 0,2 s
void move_node(Ent* e, int x, int y, float t, int direct);         // bloquea
void move_node_async(Ent* e, int x, int y, float t, int direct);
void knock_down(Ent* e);
void ent_set_tint(Ent* e, int r, int g, int b);                    // 0..255
void ent_fade(Ent* e, int to16, float t);                          // asíncrono
static inline int ent_x(Ent* e) { return PX(e->x); }
static inline int ent_y(Ent* e) { return PX(e->y); }
void ent_set_pos(Ent* e, int x, int y);
void player_reset_trail(void);
int player_facing(void);

// ------------------------------------------------------------------ mapa
int marker(const char* name, int* x, int* y);
int marker_x(const char* name);
int marker_y(const char* name);
void set_prop_sprite(const char* pid, const char* sprite);
void remove_prop(const char* pid);
void prop_dark(const char* pid);
void show_prop(const char* pid);                                   // piedras de retorno
int has_prop(const char* pid);
void pan_to(int x, int y, float wait);
void release_camera(float wait);
void shake(float amount, float t);
void set_tint(int r, int g, int b, float dur);                     // x256
void set_tint_wait(int r, int g, int b, float dur);
void show_banner(const char* text);
void world_begin(void);
void world_end(void);
void go_to_map(const char* map, const char* spawn, int door);      // no vuelve
void start_battle_enc(const char* id, const char* const* enemies, int n, const char* bg, const char* music, int boss, int flags);
void dust(int x, int y);

// ------------------------------------------------------------------ caminos (A*)
int find_path(int fx, int fy, int tx, int ty, s16* ox, s16* oy, int max);
void nav_dirty(void);

extern u8 g_scratch[];
