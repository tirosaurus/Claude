// Audio: música IMA ADPCM con fundido cruzado y efectos PCM con tono, mezclados en la interrupción de VBlank.
#pragma once
#include "core.h"

typedef struct { const char* name; const u8* data; u32 nsamples; s16 pred0; u8 idx0; u8 loop; } MusicTrack;
typedef struct { const char* name; const s8* data; u32 n; } SfxSample;
extern const MusicTrack g_music[];
extern const int g_nmusic;
extern const SfxSample g_sfx[];
extern const int g_nsfx;

// opciones (como Audio.gd)
extern u8 g_music_level, g_sfx_level, g_text_speed, g_battle_speed;

void audio_init(void);
void music_play(const char* track, float fade);
void music_stop(float fade);
const char* music_current(void);
void sfx(const char* name, float db, float pitch);
static inline void sfx1(const char* name) { sfx(name, -4.0f, 1.0f); }
float text_speed_mult(void);
float battle_speed_mult(void);
void audio_apply_settings(void);
