// Búsquedas en las tablas de datos
#include "db.h"
#include <string.h>

#define FIND(table, n, field) \
	for (int i = 0; i < (n); ++i) if (!strcmp(table[i].field, id)) return i; \
	return -1;

int db_skill(const char* id) { FIND(g_skills, N_SKILLS, id) }
int db_item(const char* id) { FIND(g_items, N_ITEMS, id) }
int db_equip(const char* id) { FIND(g_equip, N_EQUIP, id) }
int db_enemy(const char* id) { FIND(g_enemies, N_ENEMIES, key) }
int db_class(const char* id) { FIND(g_classes, N_CLASSES, id) }
int db_branch(const char* id) { FIND(g_branches, N_BRANCHES, id) }
int db_race(const char* id) { FIND(g_races, N_RACES, id) }
int db_member(const char* id) { FIND(g_members, N_MEMBERS, id) }
int db_status(const char* id) { FIND(g_status, S_COUNT, id) }

const char* db_item_name(const char* id) {
	int i = db_item(id);
	if (i >= 0) return g_items[i].name;
	i = db_equip(id);
	if (i >= 0) return g_equip[i].name;
	for (int k = 0; k < g_nupgrades; ++k) if (!strcmp(g_upgrades[k].id, id)) return g_upgrades[k].name;
	return id;
}
