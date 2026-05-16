//
// PNG Display helper class
//
// written by Larry Bank
// bitbank@pobox.com
//
// Copyright 2025 BitBank Software, Inc. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//===========================================================================
//
#ifndef __PNGDISPLAY__
#define __PNGDISPLAY__
#include <PNGdec.h>
#include <SD.h>
#ifndef ARDUINO_ARCH_NRF52
#include "FS.h"
#include <LittleFS.h>
#endif

#if !defined(__BB_EPAPER__) && !defined(SPI_LCD_H) && !defined(__FASTEPD_H__)
#error "One of the following display libraries must be included BEFORE PNGDisplay: bb_spi_lcd, bb_epaper, FastEPD"
#endif

// To center one or both coordinates for the drawing position
//  use this constant value
#define PNGDISPLAY_CENTER -2

class PNGDisplay
{
  public:
#ifdef SPI_LCD_H
    int loadPNG(BB_SPI_LCD *pLCD, int x, int y, const void *pData, int iDataSize, uint32_t bgColor = 0xffffffff);
    int loadPNG(BB_SPI_LCD *pLCD, int x, int y, const char *fname, uint32_t bgColor = 0xffffffff);
    int loadPNG_LFS(BB_SPI_LCD *pLCD, int x, int y, const char *fname, uint32_t bgColor = 0xffffffff);
#endif // SPI_LCD_H

#ifdef __FASTEPD_H__
    int loadPNG(FASTEPD *pDisplay, int x, int y, const void *pData, int iDataSize, uint32_t bgColor = 0xffffffff);
    int loadPNG(FASTEPD *pDisplay, int x, int y, const char *fname, uint32_t bgColor = 0xffffffff);
    int loadPNG_LFS(FASTEPD *pDisplay, int x, int y, const char *fname, uint32_t bgColor = 0xffffffff);
#endif // __FASTEPD_H__

#ifdef __BB_EPAPER__
    int loadPNG(BBEPAPER *pDisplay, int x, int y, const void *pData, int iDataSize, uint32_t bgColor = 0xffffffff);
    int loadPNG(BBEPAPER *pDisplay, int x, int y, const char *fname, uint32_t bgColor = 0xffffffff);
    int loadPNG_LFS(BBEPAPER *pDisplay, int x, int y, const char *fname, uint32_t bgColor = 0xffffffff); 
#endif // __BB_EPAPER__

    int getPNGInfo(int *width, int *height, int *bpp, const void *pData, int iDataSize);
    int getPNGInfo(int *width, int *height, int *bpp, const char *fname);
    int getPNGInfo_LFS(int *width, int *height, int *bpp, const char *fname);
    int getLastError() {return _iLastError;}
  private:
    int _iLastError;
};

#endif // __PNGDISPLAY__
