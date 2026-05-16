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
#ifndef __PNGDISPLAY_IMPL__
#define __PNGDISPLAY_IMPL__
#include "PNGDisplay.h"

static int PNGDraw(PNGDRAW *pDraw)
{
int *pInfo = (int *)pDraw->pUser;
uint16_t *p16;
PNG *pPNG;
#ifdef SPI_LCD_H
BB_SPI_LCD *pLCD;
    
    pLCD = (BB_SPI_LCD *)pInfo[0]; // pointer to LCD class pointer
    pPNG = (PNG *)pInfo[1]; // pointer to PNG class pointer
    p16 = (uint16_t *)&pInfo[3]; // start of temporary pixel space
    pPNG->getLineAsRGB565(pDraw, p16, PNG_RGB565_BIG_ENDIAN, (uint32_t)pInfo[2]);
    pLCD->pushPixels(p16, pDraw->iWidth);
#endif // SPI_LCD_H

#ifdef __BB_EPAPER__
BBEPAPER *pEPD;
int w, iLen, iPitch; // bits per pixel mode of the EPD
uint8_t uc_s, uc_d, srcMask, destMask, *s, *d;
int32_t iWidth, i, x, y;

    pEPD = (BBEPAPER *)pInfo[0]; // pointer to display class object
    pPNG = (PNG *)pInfo[1]; // pointer to PNG class pointer
    if (pPNG->getBpp() != 1) {
        return 0; // mismatch, don't try to convert the bit depth
    }
    d = (uint8_t *)pEPD->getBuffer();
    x = pInfo[3]; y = pInfo[4]; // display offset to draw the image
    if (pDraw->y + y > pEPD->height()) return 0; // off bottom edge
    w = pPNG->getWidth();
    if (w + x > pEPD->width()) w = pEPD->width() - x; // clip to right edge
    iPitch = (pEPD->width()+7)/8;
    d += (x/8) + ((pDraw->y + y) * iPitch);
    srcMask = 0x80;
    destMask = 0x80 >> (x & 7);
    s = pDraw->pPixels;
    i = 0;
    if ((x & 7) == 0) { // take advantage of src/dest bit alignment
        while (i < w-7) {
            *d++ = *s++;
            i += 8;
        }
    }
    uc_s = *s++;
    uc_d = d[0];
    for (; i<w; i++) { // do it pixel-by-pixel
        if (uc_s & srcMask) {
            uc_d |= destMask; // use the same pixel color polarity
        } else {
            uc_d &= ~destMask;
        }
        srcMask >>= 1;
        if (srcMask == 0) {
            srcMask = 0x80;
            uc_s = *s++;
        }
        destMask >>= 1;
        if (destMask == 0) {
            destMask = 0x80;
            *d++ = uc_d;
            uc_d = d[0];
        }
    } // for i
    if (destMask != 0x80) { // store the last partial byte
        d[0] = uc_d;
    }
#endif // __BB_EPAPER__

#ifdef __FASTEPD_H__
FASTEPD *pEPD;
int i, iLen, iPitch, iBpp; // bits per pixel mode of the EPD
uint8_t srcMask, destMask, uc_s, uc_d, *s, *d;
int32_t iWidth, w, x, y;

    pEPD = (FASTEPD *)pInfo[0]; // pointer to display class object
    pPNG = (PNG *)pInfo[1]; // pointer to PNG class pointer
    iBpp = (pEPD->getMode() == BB_MODE_1BPP) ? 1:4;
    if (pPNG->getBpp() != iBpp) return 0; // mismatch, don't try to convert the bit depth
    d = (uint8_t *)pEPD->currentBuffer();
    x = pInfo[3]; y = pInfo[4]; // display offset to draw the image
    if (pDraw->y + y > pEPD->height()) return 0; // off bottom edge
    w = pPNG->getWidth();
    if (w + x > pEPD->width()) w = pEPD->width() - x; // clip to right edge
    iPitch = (pEPD->width()*iBpp)/8;
    d += ((x * iBpp)/8) + ((pDraw->y + y) * iPitch);
    if (iBpp == 4) {
        memcpy(d, pDraw->pPixels, (w*iBpp)/8);
    } else {  // 1 bpp
        srcMask = 0x80;
        destMask = 0x80 >> (x & 7);
        s = pDraw->pPixels;
        if ((x & 7) == 0) { // take advantage of src/dest bit alignment
            while (i <= w-7) {
                *d++ = *s++;
                i += 8;
            }
        }
        uc_s = *s++;
        uc_d = d[0];
        for (; i<w; i++) { // do it pixel-by-pixel
            if (uc_s & srcMask) {
                uc_d |= destMask; // use the same pixel color polarity
            } else {
                uc_d &= ~destMask;
            }
            srcMask >>= 1;
            if (srcMask == 0) {
                srcMask = 0x80;
                uc_s = *s++;
            }
            destMask >>= 1;
            if (destMask == 0) {
                destMask = 0x80;
                *d++ = uc_d;
                uc_d = d[0];
            }
        } // for i
        if (destMask != 0) { // store the last partial byte
            d[0] = uc_d;
        }
    } // 1-bpp
#endif // __FASTEPD_H__

    return 1; // continue decoding
} /* PNGDraw() */

// Functions to access a file on the SD card

static void * pngOpen(const char *filename, int32_t *size) {
  static File myfile;
  myfile = SD.open(filename);
  *size = myfile.size();
  return &myfile;
}
#ifndef ARDUINO_ARCH_NRF52
static void * pngOpenLFS(const char *filename, int32_t *size) {
  static File myfile;
  myfile = LittleFS.open(filename, FILE_READ);
  if (myfile) {
      *size = myfile.size();
      return &myfile;
  } else {
      return NULL;
  }
}
#endif // !nRF52

static void pngClose(void *handle) {
  File *pFile = (File *)handle;
  if (pFile) pFile->close();
}
static int32_t pngRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
  File *pFile = (File *)handle->fHandle;
  if (!pFile) return 0;
  return pFile->read(buffer, length);
}
static int32_t pngSeek(PNGFILE *handle, int32_t position) {
  File *pFile = (File *)handle->fHandle;
  if (!pFile) return 0;
  return pFile->seek(position);
}

#ifdef SPI_LCD_H
int PNGDisplay::loadPNG(BB_SPI_LCD *pDisplay, int x, int y, const void *pData, int iDataSize, uint32_t bgColor)
#endif // SPI_LCD_H

#ifdef __FASTEPD_H__
int PNGDisplay::loadPNG(FASTEPD *pDisplay, int x, int y, const void *pData, int iDataSize, uint32_t bgColor)
#endif // __FASTEPD_H__

#ifdef __BB_EPAPER__
int PNGDisplay::loadPNG(BBEPAPER *pDisplay, int x, int y, const void *pData, int iDataSize, uint32_t bgColor)
#endif // __BB_EPAPER__

{
PNG *png;
int w, h, rc;
uint32_t *png_info;

    png = (PNG *)malloc(sizeof(PNG));
    if (!png) {
        _iLastError = PNG_MEM_ERROR;
        return 0;
    }
    rc = png->openRAM((uint8_t *)pData, iDataSize, PNGDraw);
    if (rc == PNG_SUCCESS) {
        w = png->getWidth();
        h = png->getHeight();
        if (x == PNGDISPLAY_CENTER) {
            x = (pDisplay->width() - w)/2;
            if (x < 0) x = 0;
        } else if (x < 0 || w + x > pDisplay->width()) {
        // clipping is not supported
            _iLastError = PNG_INVALID_PARAMETER;
            free(png);
            return 0;
        }
        if (y == PNGDISPLAY_CENTER) {
            y = (pDisplay->height() - h)/2;
            if (y < 0) y = 0;
        } else if (y < 0 || y + h > pDisplay->height()) {
            // clipping is not supported
            free(png);
            _iLastError = PNG_INVALID_PARAMETER;
            return 0;
        }
        png_info = (uint32_t *)malloc((w * sizeof(uint16_t)) + 3 * sizeof(int)); // enough for pixels and 3 32-bit values
        png_info[0] = (uint32_t)pDisplay;
        png_info[1] = (uint32_t)png;
        png_info[2] = bgColor;
        png_info[3] = x;
        png_info[4] = y;
#ifdef SPI_LCD_H
        pDisplay->setAddrWindow(x, y, w, h);
#endif
        png->decode((void *)png_info, 0); // simple decode, no options
        png->close();
        free(png);
        _iLastError = PNG_SUCCESS;
        return 1;
   }
   _iLastError = png->getLastError();
   free(png);
   return 0;
} /* loadPNG() */

#ifdef SPI_LCD_H
int PNGDisplay::loadPNG(BB_SPI_LCD *pDisplay, int x, int y, const char *fname, uint32_t bgColor)
#endif

#ifdef __FASTEPD_H__
int PNGDisplay::loadPNG(FASTEPD *pDisplay, int x, int y, const char *fname, uint32_t bgColor)
#endif

#ifdef __BB_EPAPER__
int PNGDisplay::loadPNG(BBEPAPER *pDisplay, int x, int y, const char *fname, uint32_t bgColor)
#endif

{
    PNG *png;
    int w, h, rc;
    uint32_t *png_info;

    png = (PNG *)malloc(sizeof(PNG));
    if (!png) {
        _iLastError = PNG_MEM_ERROR;
        return 0;
    }
    rc = png->open(fname, pngOpen, pngClose, pngRead, pngSeek, PNGDraw);
    if (rc == PNG_SUCCESS) {
        w = png->getWidth();
        h = png->getHeight();
        if (x == PNGDISPLAY_CENTER) {
            x = (pDisplay->width() - w)/2;
            if (x < 0) x = 0;
        } else if (x < 0 || w + x > pDisplay->width()) {
           // clipping is not supported
           free(png);
           _iLastError = PNG_INVALID_PARAMETER;
           return 0;
        }
        if (y == PNGDISPLAY_CENTER) {
            y = (pDisplay->height() - h)/2;
            if (y < 0) y = 0;
        } else if (y < 0 || y + h > pDisplay->height()) {
            free(png);
            _iLastError = PNG_INVALID_PARAMETER;
            return 0;
        }
        png_info = (uint32_t *)malloc((w * sizeof(uint16_t)) + 3 * sizeof(int)); // enough for pixels and 3 32-bit values
        png_info[0] = (uint32_t)pDisplay;
        png_info[1] = (uint32_t)png;
        png_info[2] = bgColor;
        png_info[3] = x;
        png_info[4] = y;
#ifdef SPI_LCD_H
        pDisplay->setAddrWindow(x, y, w, h);
#endif
        png->decode((void *)png_info, 0); // simple decode, no options
        png->close();
        free(png);
        _iLastError = PNG_SUCCESS;
        return 1;
    }
    _iLastError = png->getLastError();
    free(png);
    return 0;
} /* loadPNG() */

#ifndef ARDUINO_ARCH_NRF52
#ifdef SPI_LCD_H
int PNGDisplay::loadPNG_LFS(BB_SPI_LCD *pDisplay, int x, int y, const char *fname, uint32_t bgColor)
#endif

#ifdef __FASTEPD_H__
int PNGDisplay::loadPNG_LFS(FASTEPD *pDisplay, int x, int y, const char *fname, uint32_t bgColor)
#endif

#ifdef __BB_EPAPER__
int PNGDisplay::loadPNG_LFS(BBEPAPER *pDisplay, int x, int y, const char *fname, uint32_t bgColor)
#endif

{
    PNG *png;
    int w, h, rc;
    uint32_t *png_info;

    if (!LittleFS.begin(false)) {
        return 0;
    }
    png = (PNG *)malloc(sizeof(PNG));
    if (!png) {
        _iLastError = PNG_MEM_ERROR;
        return 0;
    }
    rc = png->open(fname, pngOpenLFS, pngClose, pngRead, pngSeek, PNGDraw);
    if (rc == PNG_SUCCESS) {
        w = png->getWidth();
        h = png->getHeight();
        if (x == PNGDISPLAY_CENTER) {
            x = (pDisplay->width() - w)/2;
            if (x < 0) x = 0;
        } else if (x < 0 || w + x > pDisplay->width()) {
           // clipping is not supported
           free(png);
           _iLastError = PNG_INVALID_PARAMETER;
           return 0;
        }
        if (y == PNGDISPLAY_CENTER) {
            y = (pDisplay->height() - h)/2;
            if (y < 0) y = 0;
        } else if (y < 0 || y + h > pDisplay->height()) {
            free(png);
            _iLastError = PNG_INVALID_PARAMETER;
            return 0;
        }
        png_info = (uint32_t *)malloc((w * sizeof(uint16_t)) + 3 * sizeof(int)); // enough for pixels and 3 32-bit values
        png_info[0] = (uint32_t)pDisplay;
        png_info[1] = (uint32_t)png;
        png_info[2] = bgColor;
        png_info[3] = x;
        png_info[4] = y;
#ifdef SPI_LCD_H
        pDisplay->setAddrWindow(x, y, w, h);
#endif
        png->decode((void *)png_info, 0); // simple decode, no options
        png->close();
        free(png);
        _iLastError = PNG_SUCCESS;
        return 1;
    }
    _iLastError = png->getLastError();
    free(png);
    return 0;
} /* loadPNG_LFS() */
#endif // !nRF52

int PNGDisplay::getPNGInfo(int *width, int *height, int *bpp, const void *pData, int iDataSize)
{
    PNG *png;
    int rc;
    uint32_t *png_info;

    if (!width || !height || !bpp || !pData || iDataSize < 32) return 0;
    
    png = (PNG *)malloc(sizeof(PNG));
    if (!png) {
        _iLastError = PNG_MEM_ERROR;
        return 0;
    }
    rc = png->openRAM((uint8_t *)pData, iDataSize, PNGDraw);
    if (rc == PNG_SUCCESS) {
        *width = png->getWidth();
        *height = png->getHeight();
        *bpp = png->getBpp();
        free(png);
        _iLastError = PNG_SUCCESS;
        return 1;
    }
    _iLastError = png->getLastError();
    free(png);
    return 0;
} /* getPNGInfo() */

int PNGDisplay::getPNGInfo(int *width, int *height, int *bpp, const char *fname)
{
    PNG *png;
    int rc;
    uint32_t *png_info;

    if (!width || !height || !bpp || !fname) return 0;
    png = (PNG *)malloc(sizeof(PNG));
    if (!png) {
        _iLastError = PNG_MEM_ERROR;
        return 0;
    }
    rc = png->open(fname, pngOpen, pngClose, pngRead, pngSeek, PNGDraw);
    if (rc == PNG_SUCCESS) {
        *width = png->getWidth();
        *height = png->getHeight();
        *bpp = png->getBpp();
        png->close();
        free(png);
        _iLastError = PNG_SUCCESS;
        return 1;
    }
    _iLastError = png->getLastError();
    free(png);
    return 0;
} /* getPNGInfo() */

#ifndef ARDUINO_ARCH_NRF52
int PNGDisplay::getPNGInfo_LFS(int *width, int *height, int *bpp, const char *fname)
{
    PNG *png;
    int rc;
    uint32_t *png_info;

    if (!LittleFS.begin(false)) {
        return 0;
    }
    if (!width || !height || !bpp || !fname) return 0;
    png = (PNG *)malloc(sizeof(PNG));
    if (!png) {
        _iLastError = PNG_MEM_ERROR;
        return 0;
    }
    rc = png->open(fname, pngOpenLFS, pngClose, pngRead, pngSeek, PNGDraw);
    if (rc == PNG_SUCCESS) {
        *width = png->getWidth();
        *height = png->getHeight();
        *bpp = png->getBpp();
        png->close();
        free(png);
        _iLastError = PNG_SUCCESS;
        return 1;
    }
    _iLastError = png->getLastError();
    free(png);
    return 0;
} /* getPNGInfo_LFS() */
#endif // !nRF52
#endif // __PNGDISPLAY_IMPL__
