// Caja de diálogo (port de Dialogue.gd): retrato, nombre, máquina de escribir, elecciones.
#pragma once
#include "world.h"

extern u8 g_dialog_active;
extern u32 g_dialog_close_frame;

void say_lines(const Line* lines, int n);
int choose_opts(const char* const* opts, int n, const Line* prompt);
const char* dfmt(const char* s);      // sustituye {name}, {o}, {os}, {hijo}, {el}, {e}, {raza}

#define T(s) ((Line){0, (s)})
#define P(s) ((Line){"player", (s)})
#define K(s) ((Line){"kaelen", (s)})
#define Y(s) ((Line){"yara", (s)})
#define M(s) ((Line){"mother", (s)})
#define A(s) ((Line){"aelis", (s)})
#define B(s) ((Line){"brom", (s)})
#define N(who, s) ((Line){(who), (s)})
#define SAY(...) say_lines((const Line[]){__VA_ARGS__}, sizeof((const Line[]){__VA_ARGS__}) / sizeof(Line))
#define OPTS(...) (const char* const[]){__VA_ARGS__}, sizeof((const char* const[]){__VA_ARGS__}) / sizeof(char*)
// CHOOSE(&Y("pregunta"), "a", "b")  /  CHOOSE(0, "a", "b")  /  CHOOSET("texto sin retrato", "a", "b")
#define CHOOSE(prompt, ...) choose_opts(OPTS(__VA_ARGS__), (prompt))
#define CHOOSEL(line, ...) choose_opts(OPTS(__VA_ARGS__), &(Line)line)
int choose_text(const char* prompt, const char* const* opts, int n);
#define CHOOSET(prompt, ...) choose_text((prompt), OPTS(__VA_ARGS__))
int choose_list(const char* const* opts, int n, const Line* prompt);

// listas de líneas que devuelven interact_lines/npc_lines
void lb_add(LineBuf* b, Line l);
#define LB(buf, ...) do { const Line _l[] = {__VA_ARGS__}; for (unsigned _i = 0; _i < sizeof _l / sizeof _l[0]; ++_i) lb_add((buf), _l[_i]); } while (0)
// cycle(key, opciones): cada opción es una lista de líneas terminada en {0,0}
#define END_LINES ((Line){0, 0})
#define OPT(...) ((const Line[]){__VA_ARGS__, END_LINES})
void cycle_lines(LineBuf* out, const char* key, const Line* const* opts, int n);
#define CYCLE(out, key, ...) cycle_lines((out), (key), (const Line* const[]){__VA_ARGS__}, \
	sizeof((const Line* const[]){__VA_ARGS__}) / sizeof(Line*))
void banner_tick(void);
