
#ifndef TL_DATUM
#define TL_DATUM 0
#define TC_DATUM 1
#define TR_DATUM 2
#define ML_DATUM 3
#define MC_DATUM 4
#define MR_DATUM 5
#define BL_DATUM 6
#define BC_DATUM 7
#define BR_DATUM 8
#endif

// Per-color guards: some backends (e.g. Arduino_GFX) predefine a *subset* of
// the TFT_* color names, which would otherwise skip a single combined #ifndef
// block and leave the rest undefined. Guard each one so every color is filled.
#ifndef TFT_BLACK
#define TFT_BLACK 0x0000
#endif
#ifndef TFT_NAVY
#define TFT_NAVY 0x000F
#endif
#ifndef TFT_DARKGREEN
#define TFT_DARKGREEN 0x03E0
#endif
#ifndef TFT_DARKCYAN
#define TFT_DARKCYAN 0x03EF
#endif
#ifndef TFT_MAROON
#define TFT_MAROON 0x7800
#endif
#ifndef TFT_PURPLE
#define TFT_PURPLE 0x780F
#endif
#ifndef TFT_OLIVE
#define TFT_OLIVE 0x7BE0
#endif
#ifndef TFT_LIGHTGREY
#define TFT_LIGHTGREY 0xD69A
#endif
#ifndef TFT_DARKGREY
#define TFT_DARKGREY 0x7BEF
#endif
#ifndef TFT_BLUE
#define TFT_BLUE 0x001F
#endif
#ifndef TFT_GREEN
#define TFT_GREEN 0x07E0
#endif
#ifndef TFT_CYAN
#define TFT_CYAN 0x07FF
#endif
#ifndef TFT_RED
#define TFT_RED 0xF800
#endif
#ifndef TFT_MAGENTA
#define TFT_MAGENTA 0xF81F
#endif
#ifndef TFT_YELLOW
#define TFT_YELLOW 0xFFE0
#endif
#ifndef TFT_WHITE
#define TFT_WHITE 0xFFFF
#endif
#ifndef TFT_ORANGE
#define TFT_ORANGE 0xFDA0
#endif
#ifndef TFT_GREENYELLOW
#define TFT_GREENYELLOW 0xB7E0
#endif
#ifndef TFT_PINK
#define TFT_PINK 0xFE19
#endif
#ifndef TFT_BROWN
#define TFT_BROWN 0x9A60
#endif
#ifndef TFT_GOLD
#define TFT_GOLD 0xFEA0
#endif
#ifndef TFT_SILVER
#define TFT_SILVER 0xC618
#endif
#ifndef TFT_SKYBLUE
#define TFT_SKYBLUE 0x867D
#endif
#ifndef TFT_VIOLET
#define TFT_VIOLET 0x915C
#endif
#ifndef TFT_TRANSPARENT
#define TFT_TRANSPARENT 0x0120
#endif
