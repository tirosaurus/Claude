// Registro de los guiones de cada mapa
#include "story.h"

extern const StoryDef story_bedroom, story_housemain, story_village, story_forest;

static const StoryDef* const STORIES[] = {&story_bedroom, &story_housemain, &story_village, &story_forest};

const StoryDef* story_for(const char* map) {
	for (int i = 0; i < (int)(sizeof STORIES / sizeof STORIES[0]); ++i)
		if (STORIES[i] && !strcmp(STORIES[i]->map, map)) return STORIES[i];
	return 0;
}
