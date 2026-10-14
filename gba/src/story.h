// Ayudas para los guiones de cada mapa (port de StoryBase.gd)
#pragma once
#include "dialogue.h"
#include "audio.h"
#include "scene.h"
#include <stdio.h>

#define begin story_begin
#define end story_end
void story_begin(void);
void story_end(void);
static inline void wait(float s) { wait_sec(s); }
static inline int has(int m) { return in_party(m); }
void join(int m);
void walk_party_to(int x, int y, float speed);
void rest_and_save(const char* title);
void open_save(void);
void open_shop(const char* id);
const char* give_items_list(const ChestItem* items, int n);
#define GIVE(...) give_items_list((const ChestItem[]){__VA_ARGS__}, sizeof((const ChestItem[]){__VA_ARGS__}) / sizeof(ChestItem))
void battle(const char* id, const char* const* enemies, int n, const char* bg, const char* music, int boss);
void battle_ex(const char* id, const char* const* enemies, int n, const char* bg, const char* music, int boss, int vael, int weaken_aleixolo);
#define ENEMIES(...) (const char* const[]){__VA_ARGS__}, sizeof((const char* const[]){__VA_ARGS__}) / sizeof(char*)
void place_waystone(void);
void sovereign_reminder(void);
void base_on_interact(const char* id);
void base_open_chest(const char* id);
void base_on_npc(const char* id);
int companion_banter(const char* id, LineBuf* out);
void echo_lines(const Line* lines, int n);
void track_secret(const char* id);
void flag_add(const char* key, int delta);   // contador entero en metadatos

// atajos para los guiones de mapa
static inline int tod_is(const char* t) { return !strcmp(meta_s("tod", "day"), t); }
static inline int spawn_is(const char* s) { return !strcmp(G.current_spawn, s); }
static inline void tod_set(const char* t) { meta_set_s("tod", t); }
void goto_world(const char* map, const char* spawn, float fade);   // cambia de mapa sin animación de puerta; no vuelve
void fade_npc(const char* id, float t);                            // desvanece y elimina
#define IS(a, b) (!strcmp((a), (b)))
void story_call(void (*fn)(const char*), const char* arg);
void tint_tween(Ent* e, int r, int g, int b, float t);              // bloquea
void go_ending(void);                                              // no vuelve
