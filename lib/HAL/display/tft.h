#ifndef LIB_HAL_DISPLAY_TFT_H
#define LIB_HAL_DISPLAY_TFT_H
#if !defined(USE_ARDUINO_GFX) && !defined(USE_LOVYANGFX) && !defined(USE_TFT_ESPI) && !defined(USE_M5GFX)
#define USE_TFT_ESPI
#endif

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

#ifndef TFT_BLACK
#define TFT_BLACK 0x0000
#define TFT_NAVY 0x000F
#define TFT_DARKGREEN 0x03E0
#define TFT_DARKCYAN 0x03EF
#define TFT_MAROON 0x7800
#define TFT_PURPLE 0x780F
#define TFT_OLIVE 0x7BE0
#define TFT_LIGHTGREY 0xD69A
#define TFT_DARKGREY 0x7BEF
#define TFT_BLUE 0x001F
#define TFT_GREEN 0x07E0
#define TFT_CYAN 0x07FF
#define TFT_RED 0xF800
#define TFT_MAGENTA 0xF81F
#define TFT_YELLOW 0xFFE0
#define TFT_WHITE 0xFFFF
#define TFT_ORANGE 0xFDA0
#define TFT_GREENYELLOW 0xB7E0
#define TFT_PINK 0xFE19
#define TFT_BROWN 0x9A60
#define TFT_GOLD 0xFEA0
#define TFT_SILVER 0xC618
#define TFT_SKYBLUE 0x867D
#define TFT_VIOLET 0x915C
#define TFT_TRANSPARENT 0x0120
#endif

class TFT_eSPI;
class tft_sprite;
class tft_logger;
#include <SPI.h>
#if defined(USE_TFT_ESPI)
#include "tftespi.h"

#elif defined(USE_ARDUINO_GFX)
#include "ardgfx.h"

#elif defined(USE_LOVYANGFX)
#include "lovyan.h"

#elif defined(USE_M5GFX)
#include "m5gfx.h"

#endif
#endif // LIB_HAL_DISPLAY_TFT_H
