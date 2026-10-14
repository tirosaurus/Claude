// Sustitutos temporales mientras se portan los módulos
#include "world.h"
#include "scene.h"
#include "ui.h"
#include "dialogue.h"

void battle_scene(void* a) { (void)a; scene_request(SC_WORLD); }
void ending_scene(void* a) { (void)a; scene_request(SC_TITLE); }
