// Vaelmoor para Game Boy Advance
#include "core.h"
#include "gfx.h"
#include "audio.h"
#include "scene.h"
#include "state.h"
#include "ui.h"

extern void world_scene(void* arg);
extern void title_scene(void* arg);
extern void creation_scene(void* arg);
extern void intro_scene(void* arg);
extern void battle_scene(void* arg);
extern void ending_scene(void* arg);

static TaskFn scene_fn(int sc) {
	switch (sc) {
	case SC_TITLE: return title_scene;
	case SC_CREATION: return creation_scene;
	case SC_INTRO: return intro_scene;
	case SC_WORLD: return world_scene;
	case SC_BATTLE: return battle_scene;
	case SC_ENDING: return ending_scene;
	}
	return title_scene;
}

int main(void) {
	REG_WAITCNT = 0x4317;
	REG_DISPCNT = DCNT_BLANK;
	REG_DISPSTAT = 0x0008;
	REG_IE = 0x0001;
	REG_IME = 1;
	gfx_init();
	audio_init();
	settings_load();
	rnd_seed(0x1234567u ^ REG_VCOUNT);
	g_scene_next = SC_TITLE;
	for (;;) {
		int sc = g_scene_next;
		g_scene_next = -1;
		g_scene_cur = sc;
		g_paused = 0;
		task_kill_all();
		gfx_vblank_hook = 0;
		g_frame_hook = 0;
		int id = task_spawn(scene_fn(sc), 0, TASK_BIG);
		while (task_alive(id)) {
			vsync();
			gfx_vblank();
			input_poll();
			g_frame++;
			sched_run();
		}
		if (g_scene_next < 0) g_scene_next = SC_TITLE;
	}
}
