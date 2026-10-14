// Registro de los guiones de cada mapa
#include "story.h"

extern const StoryDef story_bedroom, story_housemain, story_village, story_forest, story_village_night, story_village_dawn,
	story_cathedral, story_crypt, story_forest_deep, story_elf_camp, story_heart, story_cave, story_montes, story_minas,
	story_pantano, story_ruinas, story_paso_sur, story_camino_paso, story_senda_corazon, story_sendero_montes,
	story_sendero_pantano, story_abismo_a, story_abismo_b, story_abismo_c, story_abismo_d;

static const StoryDef* const STORIES[] = {&story_bedroom, &story_housemain, &story_village, &story_forest, &story_village_night,
	&story_village_dawn, &story_cathedral, &story_crypt, &story_forest_deep, &story_elf_camp, &story_heart, &story_cave,
	&story_montes, &story_minas, &story_pantano, &story_ruinas, &story_paso_sur, &story_camino_paso, &story_senda_corazon,
	&story_sendero_montes, &story_sendero_pantano, &story_abismo_a, &story_abismo_b, &story_abismo_c, &story_abismo_d};

const StoryDef* story_for(const char* map) {
	for (int i = 0; i < (int)(sizeof STORIES / sizeof STORIES[0]); ++i)
		if (!strcmp(STORIES[i]->map, map)) return STORIES[i];
	return 0;
}
