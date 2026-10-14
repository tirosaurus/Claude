// Registro de los guiones de cada mapa
#include "story.h"

static const StoryDef* const STORIES[] = {0};

const StoryDef* story_for(const char* map) {
	for (int i = 0; i < (int)(sizeof STORIES / sizeof STORIES[0]); ++i)
		if (STORIES[i] && !strcmp(STORIES[i]->map, map)) return STORIES[i];
	return 0;
}
