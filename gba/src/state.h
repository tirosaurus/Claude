// Estado persistente (port de GameState.gd): personaje, grupo, progreso, inventario, equipo,
// vínculos, flags de historia y guardado en la SRAM del cartucho.
#pragma once
#include "core.h"
#include "db.h"

enum { M_PLAYER, M_KAELEN, M_YARA, M_AELIS, M_BROM, M_COUNT };
enum { RACE_HUMAN, RACE_ELF, RACE_DWARF };
enum { ROM_NONE, ROM_PLAYER, ROM_KAELEN, ROM_BOTH };
#define SAVE_SLOTS 3

typedef struct {
	u8 race, female, hair_style, hair_color, skin, outfit, beard;
} Appearance;

typedef struct { s32 hp, mp; } MemberState;

typedef struct {
	s16 approval, rivalry, kaelen_approval, corruption;
	u8 alive, joined, romance, left, romance_target;
} Companion;

typedef struct { s16 item; s16 count; } InvSlot;   // item: 0..N_ITEMS-1 consumibles, N_ITEMS+ equipo

typedef struct {
	char name[16];
	u8 race, female;
	s8 branch, cls;
	u8 tree[9];
	u8 difficulty;
	Appearance app;
	s32 level, xp, gold;
	MemberState members[M_COUNT];
	Companion comp[M_COUNT];        // índice por miembro (0 sin usar)
	u8 party[M_COUNT];
	u8 nparty;
	InvSlot inv[96];
	u8 ninv;
	s16 equipment[M_COUNT][SL_N];   // índice de equipo o -1
	u8 has_equip[M_COUNT];
	u8 weapon_tier, armor_tier;
	char current_map[20];
	char current_spawn[24];
	u8 has_load_pos;
	s16 load_x, load_y;
	u32 play_frames;
	u8 auto_battle;
} GameState;

extern GameState G;

// ------------------------------------------------------------------ flags y metadatos (claves de texto)
int flag(const char* f);
void setf(const char* f);
void setf_v(const char* f, int v);
int meta_i(const char* k, int def);
void meta_set_i(const char* k, int v);
const char* meta_s(const char* k, const char* def);
void meta_set_s(const char* k, const char* v);
int meta_has(const char* k);
void meta_erase(const char* k);
void flags_clear_all(void);

// ------------------------------------------------------------------ partida
void gs_new_game(const char* name, const Appearance* app, int difficulty);
void default_appearance(Appearance* a);
const char* player_name(void);
int is_female(void);
const char* gfm(const char* masc, const char* fem);   // GameState.g()
const char* race_name(void);
const char* race_id(void);

// grupo
const char* member_id(int m);
int member_from_id(const char* id);
void join_party(int m);
void leave_party(int m);
int in_party(int m);
const char* member_name(int m);
const char* member_role(int m);
int yara_is_witch(void);
Stats7 gs_stats(int m);
int gs_skills_of(int m, s16* out);     // devuelve el número
int perk_sum(int key);                 // PK_* -> *1000
float perk(int key);

// equipo
const char* equip_rule_key(int m);
int equip_allowed(const char* rule, int eq);
int equipped(int m, int slot);
void gs_equip(int m, int slot, int eq);    // eq -1 = quitar
typedef struct { u8 weapon_kind, weapon_tier; const char* orb; const char* head; const char* body; const char* shield; } GearLook;
GearLook gear_look(int m);

// talentos
int skill_points_total(void);
int skill_points_spent(void);
int skill_points(void);
const char* talent_block_reason(int node);   // "" si se puede
int learn_talent(int node);
void level_restore(void);
int difficulty(void);
void diff_mults(float out[3]);
void heal_all(void);
typedef struct { s32 level; u8 n; u8 who[8]; s16 skill[8]; } LevelUp;
int add_xp(int amount, LevelUp* out, int max_out);
int owns(const char* id);
int missing_legendaries(void);
int vael_worthy(void);
int needs_branch_choice(void);
int needs_class_choice(void);
void choose_branch(int b);
void choose_class(int c);
const char* branch_name(void);
const char* class_name_str(void);

// inventario (ids de ITEMS o EQUIP)
int item_key(const char* id);          // índice global o -1
const char* item_key_id(int key);
void add_item(const char* id, int qty);
void remove_item(const char* id, int qty);
int item_count(const char* id);
void add_item_k(int key, int qty);
void remove_item_k(int key, int qty);
int item_count_k(int key);

// vínculos
void change_approval(int m, int delta);
void change_kaelen_rivalry(int delta);
void change_yara_kaelen_approval(int delta);
void change_yara_corruption(int delta);
int rivalry(void);
int corruption(void);
const char* kaelen_bond_label(void);
const char* yara_bond_label(void);

// guardado
typedef struct { u8 used; char name[16]; s32 level; char map[20]; u32 play_frames; u32 serial; } SlotInfo;
int save_game(int slot, int x, int y);
int load_game(int slot);
int slot_info(int slot, SlotInfo* out);
int any_save(void);
const char* format_time(u32 frames);
void settings_save(void);
void settings_load(void);
