// Interfaz: capa BG0 con ventanas (nine-patch del juego original), texto con la fuente PixelifySans,
// iconos y cursores como sprites.
#pragma once
#include "gfx.h"

#define UI_SB 30                // screenblock de BG0
// paletas de interfaz en BG
#define PAL_UI_LIGHT 15
#define PAL_UI_DARK 13
#define PAL_UI_ALT 12           // botones (menús) o ventana azul (combate)
#define PAL_UI_ALT2 11          // ventana roja (combate)
#define PAL_PORTRAIT 14

void ui_init(void);             // configura BG0 y paletas de interfaz
void ui_load_palettes(int battle);
void ui_clear(void);
void ui_clear_rect(int tx, int ty, int tw, int th);
void ui_panel(int np, int pal, int tx, int ty, int tw, int th);
void ui_fill(int tx, int ty, int tw, int th, int pal, int color);
void ui_fill_px(int x, int y, int w, int h, int color);      // sobre celdas ya existentes
void ui_pset(int x, int y, int color);
int ui_text(int x, int y, const char* s, int color, int outline);          // devuelve el ancho
int ui_text_italic(int x, int y, const char* s, int color, int outline);
int ui_text_n(int x, int y, const char* s, int nbytes, int color, int outline, int italic);
int text_width(const char* s);
int text_to_tiles(u32* tiles, int wc, int hc, int x, int y, const char* s, int color, int outline);   // a un búfer de tiles (fila a fila)
int text_width_n(const char* s, int nbytes);
int ui_text_center(int x, int y, int w, const char* s, int color, int outline);
int ui_text_right(int x, int y, int w, const char* s, int color, int outline);
// texto con ajuste de línea; devuelve el número de líneas
int wrap_text(const char* s, int maxw, char* out, int outsize);
int ui_text_wrapped(int x, int y, int maxw, int line_h, const char* s, int color, int outline, int max_lines);
void ui_portrait(const Portrait* p, int tx, int ty);         // retrato 48x48 en BG0 (paleta 14)
void ui_portrait_buf(const u32* tiles, const u16* pal, int tx, int ty);
int ui_tiles_free(void);

// sprites de interfaz (se añaden cada fotograma a la OAM)
void ui_sprites_reset(void);
void ui_icon(int x, int y, const char* name, int prio);
void ui_cursor(int x, int y, int which, int hflip);
void ui_cursor_rot(int x, int y, int which, int angle1024);
int icon_index(const char* name);

extern int g_ui_free_pal;
void ui_button(int tx, int ty, int tw, int th, const char* text, int state, int align_left, const char* icon);
void bg_image_load(const BgImage* img, int cb, int sb, int world);
int ms_load(const MetaSprite* ms, int frame);
void ms_upload(const MetaSprite* ms, int frame, int tile);
void ms_draw(const MetaSprite* ms, int tile, int pal, int x, int y, int prio, u32 key, int flip, int semi);
void ms_draw_scaled(const MetaSprite* ms, int tile, int pal, int x, int y, int prio, u32 key, int flip, int semi, int sx256, int sy256);
void ui_frame_sprites(void);
