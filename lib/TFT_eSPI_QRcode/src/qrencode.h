#ifndef __AVR__
#define PROGMEM
#define memcpy_P memcpy
#define __LPM(x) *x
#define pgm_read_word_qr(x) *x
#else
#include <avr/pgmspace.h>
#define USEPRECALC
#endif

extern unsigned char strinbuf[];
extern unsigned char *qrframe;

extern unsigned char  WD, WDB;
#include "qrbits.h"

#ifdef __cplusplus
extern "C"{
#endif
// strinbuf in, qrframe out
void qrencode(void);
#ifdef __cplusplus
} // extern "C"
#endif


