#include "scene.h"
#include "gfx.h"

int g_scene_next = -1;
int g_scene_cur = -1;
Encounter g_enc;

void scene_request(int sc) { g_scene_next = sc; }
int scene_requested(void) { return g_scene_next >= 0; }
void block_forever(void) { for (;;) frame_wait(); }
