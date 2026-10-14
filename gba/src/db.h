// Tablas de datos del juego (generadas desde DB.gd por tools/conv_db.py)
#pragma once
#include "mapdata.h"
#include "../build/gen/db_ids.h"

enum { ST_HP, ST_MP, ST_ATK, ST_DEF, ST_MAG, ST_RES, ST_SPD, ST_N };
enum { T_ENEMY, T_ENEMIES, T_ALLY, T_ALLIES, T_SELF, T_ALLY_DEAD, T_ENEMIES_RANDOM };
enum { K_PHYS, K_MAG, K_HEAL, K_BUFF, K_STATUS, K_REVIVE, K_STEAL, K_CURE, K_DRAIN, K_SUMMON, K_EAT };
enum { EL_NONE, EL_FIRE, EL_ICE, EL_BOLT, EL_LIGHT, EL_DARK, EL_NATURE, EL_PHYS };
enum { SL_WEAPON, SL_SHIELD, SL_HEAD, SL_BODY, SL_RING, SL_N };
enum { WK_NONE, WK_SWORD, WK_AXE, WK_DAGGERS, WK_STAFF, WK_BOW };
enum { WT_NONE, WT_LIGHT, WT_MEDIUM, WT_HEAVY, WT_CLOTH };
enum { PK_NONE, PK_CRIT, PK_HEAL_POW, PK_MP_REGEN, PK_DMG_PHYS, PK_DMG_MAG, PK_DMG_RED, PK_LIFESTEAL, PK_ATB_START };

typedef struct { s32 v[7]; } Stats7;
typedef struct { float v[7]; } StatsF;

typedef struct { const char* id; const char* name; const char* icon; } StatusDef;
typedef struct {
	const char* id; const char* name; const char* desc;
	s16 mp; u8 target, kind, el, hits;
	float power, base, crit, chance, ratio, heal_party, vs_poison;
	s8 inflict;
	u8 status[3]; u8 nstatus;
	u8 self_status[3]; u8 nself;
	u8 add_status[3]; u8 nadd;
	u8 turns, cure, has_heal_party, has_vs_poison;
} SkillDef;
typedef struct {
	const char* id; const char* name; const char* desc; const char* icon;
	s16 price; u8 target; s16 heal, mp; u8 cure; float revive; u8 has_revive; s16 damage; u8 el;
	const char* special; u8 key, nap;
} ItemDef;
typedef struct { const char* id; const char* name; u8 col[3]; } RarityDef;
typedef struct {
	const char* id; u8 rarity, lvl; const char* name; u8 slot, kind, tier; s16 price;
	Stats7 stats; u8 stat_mask; u8 order[7]; u8 norder;
	const char* look; u8 weight; const char* orb; const char* lore;
} EquipDef;
typedef struct { const char* id; const char* name; const char* desc; const char* trait; Stats7 base; StatsF grow; } RaceDef;
typedef struct { const char* id; const char* name; const char* desc; const char* icon; Stats7 bonus; u8 classes[3]; } BranchDef;
typedef struct {
	const char* id; u8 tier, max; s16 skill; Stats7 stats; u8 has_stats; u8 perk; float perk_val;
	const char* name; const char* desc;
} TreeNode;
typedef struct {
	const char* id; const char* name; const char* desc; const char* icon; u8 weapon; const char* orb;
	Stats7 bonus; s16 starter; TreeNode tree[9];
} ClassDef;
typedef struct {
	const char* id; const char* name; const char* role; Stats7 base; StatsF grow;
	u8 skill_lv[6]; s16 skill[6]; u8 nskills;
	u8 dark_lv[6]; s16 dark[6]; u8 ndark;
} MemberDef;
typedef struct { s16 skill; u8 weight; } AiEntry;
typedef struct { const char* item; float chance; } DropDef;
typedef struct {
	const char* key; const char* name; s8 frames; float scale;
	s32 hp, atk, def, mag, res, spd, xp, gold;
	u16 weak, resist;
	AiEntry ai[5]; u8 nai; u8 boss;
	DropDef drops[2]; u8 ndrops;
	float tint[3]; u8 has_tint;
	const char* summon; const char* summon_msg;
} EnemyDef;
typedef struct { const char* key; u8 weapons; u8 shield; u8 weights; } EquipRule;
typedef struct { const char* key; const char* items[4]; u8 n; } GearList;
typedef struct { const char* id; const char* name; const char* const* stock; u8 n; } ShopDef;
typedef struct { const char* item; s16 qty; } ChestItem;
typedef struct { const char* id; ChestItem items[4]; u8 n; } ChestDef;
typedef struct { const char* id; const char* name; const char* desc; s16 price; u8 weapon; u8 tier; } UpgradeDef;
typedef struct { const char* name; u8 rgb[3]; } ColorOpt;
typedef struct { const char* id; const char* name; } NameOpt;

extern const StatusDef g_status[];
extern const SkillDef g_skills[];
extern const ItemDef g_items[];
extern const RarityDef g_rarity[];
extern const EquipDef g_equip[];
extern const RaceDef g_races[];
extern const BranchDef g_branches[];
extern const ClassDef g_classes[];
extern const MemberDef g_members[];
extern const EnemyDef g_enemies[];
extern const EquipRule g_equip_rules[]; extern const int g_nequip_rules;
extern const GearList g_class_gear[]; extern const int g_class_gear_n;
extern const GearList g_member_gear[]; extern const int g_member_gear_n;
extern const ShopDef g_shops[]; extern const int g_nshops;
extern const ChestDef g_chests[]; extern const int g_nchests;
extern const UpgradeDef g_upgrades[]; extern const int g_nupgrades;
extern const char* const g_legendaries[]; extern const int g_nlegendaries;
extern const char* const g_vael_set[];
extern const char* const g_slot_names[];
extern const char* const g_stat_names[];
extern const ColorOpt g_hair_colors[], g_skin_tones[], g_outfits[];
extern const NameOpt g_hair_styles[];

// búsquedas por identificador (-1 si no existe)
int db_skill(const char* id);
int db_item(const char* id);      // objetos consumibles (ITEMS)
int db_equip(const char* id);     // piezas de equipo (EQUIP)
int db_enemy(const char* id);
int db_class(const char* id);
int db_branch(const char* id);
int db_race(const char* id);
int db_member(const char* id);
int db_status(const char* id);
const char* db_item_name(const char* id);   // nombre de objeto o equipo
int xp_to_next(int level);
