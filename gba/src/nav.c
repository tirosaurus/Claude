// Caminos que rodean paredes, casas y objetos (port de World.find_path con AStarGrid2D).
#include "world.h"

#define NAV_CELL 8
static u8 nav_ok;
static int cols, rows;
static u8 solid[(128 * 128) / 8] EWRAM_BSS;

void nav_dirty(void) { nav_ok = 0; }

static inline int is_solid(int x, int y) {
	if (x < 0 || y < 0 || x >= cols || y >= rows) return 1;
	int i = y * cols + x;
	return (solid[i >> 3] >> (i & 7)) & 1;
}
static inline void set_solid(int x, int y) { int i = y * cols + x; solid[i >> 3] |= 1 << (i & 7); }

static void mark_rect(int rx, int ry, int rw, int rh) {
	// grow_individual(6, 4, 6, 3)
	int x0 = rx - 6, y0 = ry - 4, x1 = rx + rw + 6, y1 = ry + rh + 3;
	int cx0 = CLAMP(x0 >= 0 ? x0 / NAV_CELL : -1, 0, cols - 1);
	int cy0 = CLAMP(y0 >= 0 ? y0 / NAV_CELL : -1, 0, rows - 1);
	int cx1 = CLAMP(x1 / NAV_CELL, 0, cols - 1);
	int cy1 = CLAMP(y1 / NAV_CELL, 0, rows - 1);
	for (int y = cy0; y <= cy1; ++y)
		for (int x = cx0; x <= cx1; ++x) set_solid(x, y);
}

static void build(void) {
	cols = (W.m->w + NAV_CELL - 1) / NAV_CELL;
	rows = (W.m->h + NAV_CELL - 1) / NAV_CELL;
	if (cols > 128) cols = 128;
	if (rows * cols > 128 * 128) rows = 128 * 128 / cols;
	memset(solid, 0, (cols * rows + 7) / 8);
	for (int i = 0; i < W.m->nsolids; ++i) {
		const Rect16* r = &W.m->solids[i];
		mark_rect(r->x, r->y, r->w, r->h);
	}
	for (int i = 0; i < W.m->nprops; ++i) {
		const MapProp* p = &W.m->props[i];
		if (!p->has_col || W.prop_removed[i]) continue;
		if (p->patch >= 0 && W.m->patches[p->patch].kind == 2 && !W.pstate[p->patch]) continue;
		mark_rect(p->x + p->col.x, p->y + p->col.y, p->col.w, p->col.h);
	}
	nav_ok = 1;
}

static void free_cell_near(int* cx, int* cy) {
	if (!is_solid(*cx, *cy)) return;
	for (int r = 1; r < 12; ++r)
		for (int dy = -r; dy <= r; ++dy)
			for (int dx = -r; dx <= r; ++dx) {
				int x = *cx + dx, y = *cy + dy;
				if (x >= 0 && y >= 0 && x < cols && y < rows && !is_solid(x, y)) { *cx = x; *cy = y; return; }
			}
}

// ------------------------------------------------------------------ A*
// memoria en g_scratch: g (u16), padre (u16), estado (u8), montículo (u32)
#define MAXN (128 * 60)
static u16* gcost;
static u16* parent;
static u8* state;            // 0 nuevo, 1 abierto, 2 cerrado
static u32* heap;
static int heap_n;

static void heap_push(u32 v) {
	int i = heap_n++;
	while (i > 0) {
		int p = (i - 1) >> 1;
		if (heap[p] <= v) break;
		heap[i] = heap[p];
		i = p;
	}
	heap[i] = v;
}
static u32 heap_pop(void) {
	u32 top = heap[0], v = heap[--heap_n];
	int i = 0;
	for (;;) {
		int l = i * 2 + 1;
		if (l >= heap_n) break;
		int c = (l + 1 < heap_n && heap[l + 1] < heap[l]) ? l + 1 : l;
		if (heap[c] >= v) break;
		heap[i] = heap[c];
		i = c;
	}
	heap[i] = v;
	return top;
}

static int heur(int ax, int ay, int bx, int by) {
	int dx = ABS(ax - bx), dy = ABS(ay - by);
	return dx > dy ? 10 * dx + 4 * dy : 10 * dy + 4 * dx;
}

int find_path(int fx, int fy, int tx, int ty, s16* ox, s16* oy, int max) {
	if (!nav_ok) build();
	int ax = CLAMP(fx / NAV_CELL, 0, cols - 1), ay = CLAMP(fy / NAV_CELL, 0, rows - 1);
	int bx = CLAMP(tx / NAV_CELL, 0, cols - 1), by = CLAMP(ty / NAV_CELL, 0, rows - 1);
	free_cell_near(&ax, &ay);
	free_cell_near(&bx, &by);
	int n = cols * rows;
	if (n > MAXN) { ox[0] = tx; oy[0] = ty; return 1; }
	gcost = (u16*)g_scratch;
	parent = gcost + n;
	state = (u8*)(parent + n);
	heap = (u32*)(((u32)(state + n) + 3) & ~3u);
	memset(state, 0, n);
	heap_n = 0;
	int start = ay * cols + ax, goal = by * cols + bx;
	gcost[start] = 0;
	parent[start] = start;
	state[start] = 1;
	heap_push(((u32)heur(ax, ay, bx, by) << 16) | start);
	int found = 0;
	static const s8 NB[8][3] = {{1, 0, 10}, {-1, 0, 10}, {0, 1, 10}, {0, -1, 10}, {1, 1, 14}, {1, -1, 14}, {-1, 1, 14}, {-1, -1, 14}};
	int max_heap = (65536 - (n * 5 + 8)) / 4;
	while (heap_n) {
		u32 top = heap_pop();
		int cur = top & 0xFFFF;
		if (state[cur] == 2) continue;
		state[cur] = 2;
		if (cur == goal) { found = 1; break; }
		int cx = cur % cols, cy = cur / cols;
		for (int k = 0; k < 8; ++k) {
			int nx = cx + NB[k][0], ny = cy + NB[k][1];
			if (is_solid(nx, ny)) continue;
			if (k >= 4 && (is_solid(cx + NB[k][0], cy) || is_solid(cx, cy + NB[k][1]))) continue;
			int ni = ny * cols + nx;
			if (state[ni] == 2) continue;
			int g = gcost[cur] + NB[k][2];
			if (state[ni] == 1 && gcost[ni] <= g) continue;
			gcost[ni] = g;
			parent[ni] = cur;
			state[ni] = 1;
			if (heap_n < max_heap) heap_push(((u32)(g + heur(nx, ny, bx, by)) << 16) | ni);
		}
	}
	if (!found) { ox[0] = tx; oy[0] = ty; return 1; }
	// reconstruir (de la meta al inicio) en un búfer temporal
	static s16 cxs[512] EWRAM_BSS, cys[512] EWRAM_BSS;
	int len = 0;
	for (int c = goal; ; c = parent[c]) {
		if (len < 512) { cxs[len] = (c % cols) * NAV_CELL + NAV_CELL / 2; cys[len] = (c / cols) * NAV_CELL + NAV_CELL / 2; len++; }
		if (c == start) break;
	}
	// invertir
	for (int i = 0; i < len / 2; ++i) {
		s16 t = cxs[i]; cxs[i] = cxs[len - 1 - i]; cxs[len - 1 - i] = t;
		t = cys[i]; cys[i] = cys[len - 1 - i]; cys[len - 1 - i] = t;
	}
	// simplificar: quitar puntos intermedios alineados
	static s16 sx[512] EWRAM_BSS, sy[512] EWRAM_BSS;
	int sn = 0;
	sx[sn] = cxs[0]; sy[sn] = cys[0]; sn++;
	for (int i = 1; i < len - 1; ++i) {
		int d1x = cxs[i] - sx[sn - 1], d1y = cys[i] - sy[sn - 1];
		int d2x = cxs[i + 1] - cxs[i], d2y = cys[i + 1] - cys[i];
		// direcciones normalizadas distintas
		int same = (d1x * d2y == d1y * d2x) && (d1x * d2x >= 0) && (d1y * d2y >= 0);
		if (!same) { sx[sn] = cxs[i]; sy[sn] = cys[i]; sn++; }
	}
	if (len > 1) { sx[sn] = cxs[len - 1]; sy[sn] = cys[len - 1]; sn++; }
	int out = 0;
	for (int i = 1; i < sn && out < max; ++i) { ox[out] = sx[i]; oy[out] = sy[i]; out++; }
	int tcx = CLAMP(tx / NAV_CELL, 0, cols - 1), tcy = CLAMP(ty / NAV_CELL, 0, rows - 1);
	if (!is_solid(tcx, tcy)) {
		if (out > 0) { ox[out - 1] = tx; oy[out - 1] = ty; }
		else if (max > 0) { ox[0] = tx; oy[0] = ty; out = 1; }
	}
	if (!out) { ox[0] = tx; oy[0] = ty; out = 1; }
	return out;
}
