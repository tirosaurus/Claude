// Núcleo: tareas cooperativas, entrada, azar, cadenas temporales y utilidades matemáticas.
#pragma once
#include "gba.h"
#include <string.h>
#include <stdarg.h>

#define EWRAM_BSS __attribute__((section(".sbss")))
#define ARRLEN(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))
#define ABS(a) ((a) < 0 ? -(a) : (a))

// ------------------------------------------------------------------ entrada
extern u16 g_keys, g_keys_prev;
void input_poll(void);
static inline int key_held(u16 k) { return (g_keys & k) != 0; }
static inline int key_hit(u16 k) { return (g_keys & ~g_keys_prev & k) != 0; }
int key_rep(u16 k);              // pulsación con repetición automática (menús)
void input_flush(void);          // olvida las pulsaciones de este fotograma

// ------------------------------------------------------------------ azar
u32 rnd(void);
int randi(int lo, int hi);       // [lo, hi]
float randf(void);               // [0, 1)
float randf_range(float a, float b);
void rnd_seed(u32 s);

// ------------------------------------------------------------------ tiempo y tareas
#define FPS 60
extern u32 g_frame;
extern u8 g_paused;              // pausa de las tareas "pausables" (menú de pausa)
typedef void (*TaskFn)(void* arg);
enum { TASK_PAUSABLE = 1, TASK_BIG = 2 };
int task_spawn(TaskFn fn, void* arg, int flags);
int task_set_pausable(int on);   // de la tarea actual; devuelve el valor anterior
extern u8 g_menu_open;           // un guion tiene abierto un menú: el mundo ni se actualiza ni se dibuja
int task_alive(int id);
void task_kill(int id);
int task_current(void);
void task_kill_pausable(void);
void task_kill_all(void);   // al cambiar de escena: mata guiones pendientes
void frame_wait(void);           // cede hasta el siguiente fotograma
void wait_frames(int n);
void wait_sec(float s);
void sched_run(void);

// gancho que se ejecuta una vez por fotograma antes de las tareas (lo pone la escena activa)
extern void (*g_frame_hook)(void);

// ------------------------------------------------------------------ cadenas
const char* sfmt(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
int utf8_next(const char** s);
int streq(const char* a, const char* b);
int starts_with(const char* s, const char* p);
int ends_with(const char* s, const char* p);

// ------------------------------------------------------------------ matemáticas (punto fijo)
u32 isqrt(u32 v);
int fsin(int a);                 // a: 0..1023 una vuelta; resultado *4096
int fcos(int a);
float sinf_fast(float x);        // radianes
int roundf_i(float x);
