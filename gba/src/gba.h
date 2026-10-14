// Registros y utilidades de la Game Boy Advance (sin dependencias externas).
#pragma once
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

#define IWRAM_CODE __attribute__((section(".iwram"), long_call))
#define EWRAM_DATA __attribute__((section(".ewram")))

#define REG_DISPCNT   (*(volatile u16*)0x04000000)
#define REG_DISPSTAT  (*(volatile u16*)0x04000004)
#define REG_VCOUNT    (*(volatile u16*)0x04000006)
#define REG_BG0CNT    (*(volatile u16*)0x04000008)
#define REG_BG1CNT    (*(volatile u16*)0x0400000A)
#define REG_BG2CNT    (*(volatile u16*)0x0400000C)
#define REG_BG3CNT    (*(volatile u16*)0x0400000E)
#define REG_BG0HOFS   (*(volatile u16*)0x04000010)
#define REG_BG0VOFS   (*(volatile u16*)0x04000012)
#define REG_BG1HOFS   (*(volatile u16*)0x04000014)
#define REG_BG1VOFS   (*(volatile u16*)0x04000016)
#define REG_BG2HOFS   (*(volatile u16*)0x04000018)
#define REG_BG2VOFS   (*(volatile u16*)0x0400001A)
#define REG_BLDCNT    (*(volatile u16*)0x04000050)
#define REG_BLDALPHA  (*(volatile u16*)0x04000052)
#define REG_BLDY      (*(volatile u16*)0x04000054)
#define REG_KEYINPUT  (*(volatile u16*)0x04000130)
#define REG_IE        (*(volatile u16*)0x04000200)
#define REG_IF        (*(volatile u16*)0x04000202)
#define REG_WAITCNT   (*(volatile u16*)0x04000204)
#define REG_IME       (*(volatile u16*)0x04000208)

#define REG_DMA3SAD   (*(volatile u32*)0x040000D4)
#define REG_DMA3DAD   (*(volatile u32*)0x040000D8)
#define REG_DMA3CNT   (*(volatile u32*)0x040000DC)

#define PAL_BG   ((volatile u16*)0x05000000)
#define PAL_OBJ  ((volatile u16*)0x05000200)
#define VRAM     ((volatile u16*)0x06000000)
#define OAM      ((volatile u16*)0x07000000)

#define CHARBLOCK(n)   ((volatile u32*)(0x06000000 + (n) * 0x4000))
#define SCREENBLOCK(n) ((volatile u16*)(0x06000000 + (n) * 0x800))
#define OBJ_TILES      ((volatile u32*)0x06010000)

#define DCNT_MODE0  0x0000
#define DCNT_OBJ1D  0x0040
#define DCNT_BLANK  0x0080
#define DCNT_BG0    0x0100
#define DCNT_BG1    0x0200
#define DCNT_BG2    0x0400
#define DCNT_BG3    0x0800
#define DCNT_OBJ    0x1000

// Teclas (los bits del registro van a 0 cuando se pulsan)
#define KEY_A      0x0001
#define KEY_B      0x0002
#define KEY_SELECT 0x0004
#define KEY_START  0x0008
#define KEY_RIGHT  0x0010
#define KEY_LEFT   0x0020
#define KEY_UP     0x0040
#define KEY_DOWN   0x0080
#define KEY_R      0x0100
#define KEY_L      0x0200

static inline u16 rgb15(int r, int g, int b) { return (r & 31) | ((g & 31) << 5) | ((b & 31) << 10); }

static inline void dma_copy32(volatile void* dst, const void* src, u32 words) {
	REG_DMA3SAD = (u32)src;
	REG_DMA3DAD = (u32)dst;
	REG_DMA3CNT = words | 0x84000000;   // 32 bits, activar
}

static inline void dma_fill32(volatile void* dst, u32 value, u32 words) {
	static volatile u32 v;
	v = value;
	REG_DMA3SAD = (u32)&v;
	REG_DMA3DAD = (u32)dst;
	REG_DMA3CNT = words | 0x85000000;   // fuente fija
}

// Espera al siguiente VBlank usando la BIOS (bajo consumo)
static inline void vsync(void) {
	__asm__ volatile("swi 0x05" ::: "r0", "r1", "r2", "r3", "memory");
}

// Entrada
extern u16 g_keys, g_keys_prev;
static inline void keys_poll(void) { g_keys_prev = g_keys; g_keys = ~REG_KEYINPUT & 0x03FF; }
static inline int key_held(u16 k) { return (g_keys & k) != 0; }
static inline int key_hit(u16 k) { return (g_keys & ~g_keys_prev & k) != 0; }
