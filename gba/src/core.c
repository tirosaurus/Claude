#include "core.h"
#include <stdio.h>

u16 g_keys, g_keys_prev;
u32 g_frame;
u8 g_paused;
void (*g_frame_hook)(void);
static u16 rep_timer[10];
static u16 rep_mask;

void input_poll(void) {
	g_keys_prev = g_keys;
	g_keys = ~REG_KEYINPUT & 0x03FF;
	rep_mask = 0;
	for (int i = 0; i < 10; ++i) {
		u16 b = 1 << i;
		if (g_keys & b) {
			if (!(g_keys_prev & b)) { rep_timer[i] = 0; rep_mask |= b; }
			else if (++rep_timer[i] >= 18 && (rep_timer[i] - 18) % 5 == 0) rep_mask |= b;
		}
	}
}

int key_rep(u16 k) { return (rep_mask & k) != 0; }

void input_flush(void) { g_keys_prev = g_keys; rep_mask = 0; }

// ------------------------------------------------------------------ azar (xorshift32)
static u32 rng_state = 0x9E3779B9;
void rnd_seed(u32 s) { rng_state = s ? s : 0x9E3779B9; }
u32 rnd(void) {
	u32 x = rng_state;
	x ^= x << 13; x ^= x >> 17; x ^= x << 5;
	return rng_state = x;
}
int randi(int lo, int hi) { return hi <= lo ? lo : lo + (int)(rnd() % (u32)(hi - lo + 1)); }
float randf(void) { return (rnd() >> 8) * (1.0f / 16777216.0f); }
float randf_range(float a, float b) { return a + (b - a) * randf(); }

// ------------------------------------------------------------------ tareas
extern void ctx_switch(u32* save, u32* load);
#define MAX_TASKS 10
#define BIG_STACK 6144
#define SMALL_STACK 2048
#define N_BIG 4
typedef struct {
	u32 regs[10];
	TaskFn fn;
	void* arg;
	u8 alive, flags, big;
	u8* stack;
	u32 stack_size;
} Task;
static Task tasks[MAX_TASKS];
static u32 sched_regs[10];
static int cur = -1;
static u8 big_stacks[N_BIG][BIG_STACK] __attribute__((aligned(8))) EWRAM_BSS;
static u8 small_stacks[MAX_TASKS - N_BIG][SMALL_STACK] __attribute__((aligned(8))) EWRAM_BSS;

static void task_entry(void) {
	Task* t = &tasks[cur];
	t->fn(t->arg);
	t->alive = 0;
	ctx_switch(t->regs, sched_regs);
	for (;;) {}
}

int task_spawn(TaskFn fn, void* arg, int flags) {
	int lo = (flags & TASK_BIG) ? 0 : N_BIG, hi = (flags & TASK_BIG) ? N_BIG : MAX_TASKS;
	for (int pass = 0; pass < 2; ++pass) {
		for (int i = lo; i < hi; ++i) {
			if (tasks[i].alive) continue;
			Task* t = &tasks[i];
			t->fn = fn; t->arg = arg; t->flags = flags; t->alive = 1;
			t->stack = i < N_BIG ? big_stacks[i] : small_stacks[i - N_BIG];
			t->stack_size = i < N_BIG ? BIG_STACK : SMALL_STACK;
			memset(t->regs, 0, sizeof t->regs);
			t->regs[8] = (u32)(t->stack + t->stack_size);   // sp
			t->regs[9] = (u32)task_entry;                     // lr
			return i;
		}
		// sin hueco pequeño: usar uno grande
		lo = 0; hi = MAX_TASKS;
	}
	return -1;
}

int task_alive(int id) { return id >= 0 && id < MAX_TASKS && tasks[id].alive; }
void task_kill(int id) {
	if (id >= 0 && id < MAX_TASKS && id != cur) tasks[id].alive = 0;
}
int task_current(void) { return cur; }
int task_set_pausable(int on) {
	if (cur < 0) return 0;
	int old = tasks[cur].flags & TASK_PAUSABLE;
	if (on) tasks[cur].flags |= TASK_PAUSABLE; else tasks[cur].flags &= ~TASK_PAUSABLE;
	return old;
}
u8 g_menu_open;

void task_kill_all(void) {
	for (int i = 0; i < MAX_TASKS; ++i) if (i != cur) tasks[i].alive = 0;
}

void task_kill_pausable(void) {
	for (int i = 0; i < MAX_TASKS; ++i)
		if (tasks[i].alive && (tasks[i].flags & TASK_PAUSABLE) && i != cur) tasks[i].alive = 0;
}

void frame_wait(void) {
	if (cur < 0) return;          // desde el contexto principal no se puede ceder
	int me = cur;
	ctx_switch(tasks[me].regs, sched_regs);
}

void wait_frames(int n) { while (n-- > 0) frame_wait(); }
void wait_sec(float s) { wait_frames((int)(s * FPS + 0.5f)); }

void sched_run(void) {
	if (g_frame_hook) g_frame_hook();
	for (int i = 0; i < MAX_TASKS; ++i) {
		if (!tasks[i].alive) continue;
		if (g_paused && (tasks[i].flags & TASK_PAUSABLE)) continue;
		cur = i;
		ctx_switch(sched_regs, tasks[i].regs);
		cur = -1;
	}
}

// ------------------------------------------------------------------ cadenas temporales
static char arena[4096] EWRAM_BSS;
static int arena_pos;
const char* sfmt(const char* fmt, ...) {
	char tmp[512];
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
	va_end(ap);
	if (n < 0) n = 0;
	if (n > (int)sizeof tmp - 1) n = sizeof tmp - 1;
	if (arena_pos + n + 1 > (int)sizeof arena) arena_pos = 0;
	char* out = arena + arena_pos;
	memcpy(out, tmp, n);
	out[n] = 0;
	arena_pos += n + 1;
	return out;
}

int utf8_next(const char** s) {
	const u8* p = (const u8*)*s;
	int c = *p++;
	if (c >= 0xC0 && c < 0xE0) c = ((c & 0x1F) << 6) | (*p++ & 0x3F);
	else if (c >= 0xE0 && c < 0xF0) { c = ((c & 0x0F) << 12) | ((p[0] & 0x3F) << 6) | (p[1] & 0x3F); p += 2; }
	else if (c >= 0xF0) { c = '?'; p += 3; }
	*s = (const char*)p;
	return c;
}

int streq(const char* a, const char* b) { return a && b && !strcmp(a, b); }
int starts_with(const char* s, const char* p) { return s && !strncmp(s, p, strlen(p)); }
int ends_with(const char* s, const char* p) {
	if (!s) return 0;
	int a = strlen(s), b = strlen(p);
	return a >= b && !strcmp(s + a - b, p);
}

// ------------------------------------------------------------------ matemáticas
u32 isqrt(u32 v) {
	u32 r = 0, b = 1u << 30;
	while (b > v) b >>= 2;
	while (b) {
		if (v >= r + b) { v -= r + b; r = (r >> 1) + b; }
		else r >>= 1;
		b >>= 2;
	}
	return r;
}

static s16 sin_tab[257];
static int sin_ready;
static void sin_init(void) {
	// cuarto de onda por serie de Taylor en punto flotante (una sola vez)
	for (int i = 0; i <= 256; ++i) {
		float x = i * (3.14159265f / 512.0f);
		float x2 = x * x;
		float s = x * (1 - x2 / 6 * (1 - x2 / 20 * (1 - x2 / 42 * (1 - x2 / 72))));
		sin_tab[i] = (s16)(s * 4096 + 0.5f);
	}
	sin_ready = 1;
}
int fsin(int a) {
	if (!sin_ready) sin_init();
	a &= 1023;
	if (a < 256) return sin_tab[a];
	if (a < 512) return sin_tab[512 - a];
	if (a < 768) return -sin_tab[a - 512];
	return -sin_tab[1024 - a];
}
int fcos(int a) { return fsin(a + 256); }
float sinf_fast(float x) {
	int a = (int)(x * (1024.0f / 6.2831853f));
	return fsin(a) / 4096.0f;
}
int roundf_i(float x) { return x >= 0 ? (int)(x + 0.5f) : -(int)(-x + 0.5f); }
