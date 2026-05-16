//
//  main.cpp
//  pngdec_test
//
//  Created by Larry Bank on 5/9/21.
//

#include "../src/PNGdec.h"

PNG png; // static instance of class

/* Windows BMP header info (54 bytes) */
uint8_t winbmphdr[54] =
        {0x42,0x4d,
         0,0,0,0,         /* File size */
         0,0,0,0,0x36,4,0,0,0x28,0,0,0,
         0,0,0,0, /* Xsize */
         0,0,0,0, /* Ysize */
         1,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       /* number of planes, bits per pel */
         0,0,0,0};

//
// Minimal code to save frames as Windows BMP files
//
void SaveBMP(char *fname, uint8_t *pBitmap, uint8_t *pPalette, int cx, int cy, int bpp)
{
FILE * oHandle;
int i, bsize, lsize;
uint32_t *l;
uint8_t *s;
uint8_t ucTemp[1024];

    oHandle = fopen(fname, "w+b");
    bsize = (cx * bpp) >> 3;
    lsize = (bsize + 3) & 0xfffc; /* Width of each line */
    winbmphdr[26] = 1; // number of planes
    winbmphdr[28] = (uint8_t)bpp;

   /* Write the BMP header */
   l = (uint32_t *)&winbmphdr[2];
    i =(cy * lsize) + 54;
    if (bpp <= 8)
        i += 1024;
   *l = (uint32_t)i; /* Store the file size */
   l = (uint32_t *)&winbmphdr[10]; // pointer to data
    *l = i - (cy * lsize);
    winbmphdr[14] = 0x28;
   l = (uint32_t *)&winbmphdr[18];
   *l = (uint32_t)cx;      /* width */
   *(l+1) = (uint32_t)(-cy);  /* height */
   fwrite(winbmphdr, 1, 54, oHandle);
    if (bpp <= 8) {
    if (pPalette == NULL) {// create a grayscale palette
        int iDelta, iCount = 1<<bpp;
        int iGray = 0;
        iDelta = 255/(iCount-1);
        for (i=0; i<iCount; i++) {
            ucTemp[i*4+0] = (uint8_t)iGray;
            ucTemp[i*4+1] = (uint8_t)iGray;
            ucTemp[i*4+2] = (uint8_t)iGray;
            ucTemp[i*4+3] = 0;
            iGray += iDelta;
        }
    } else {
        for (i=0; i<256; i++) // change palette to WinBMP format
        {
            ucTemp[i*4 + 0] = pPalette[(i*3)+2];
            ucTemp[i*4 + 1] = pPalette[(i*3)+1];
            ucTemp[i*4 + 2] = pPalette[(i*3)+0];
            ucTemp[i*4 + 3] = 0;
        }
    }
    fwrite(ucTemp, 1, 1024, oHandle);
    }
   /* Write the image data */
   for (i=0; i<cy; i++)
    {
        s = &pBitmap[i*bsize];
        if (bpp >= 24) { // swap R/B for Windows BMP byte order
            uint8_t ucTemp[2048];
            int j, iBpp = bpp/8;
            uint8_t *d = ucTemp;
            for (j=0; j<cx; j++) {
                d[0] = s[2]; d[1] = s[1]; d[2] = s[0];
                d += iBpp; s += iBpp;
            }
            fwrite(ucTemp, 1, (size_t)lsize, oHandle);
        } else {
            fwrite(s, 1, (size_t)lsize, oHandle);
        }
    }
   fclose(oHandle);
} /* SaveBMP() */

void PNGDraw(PNGDRAW *pDraw)
{
    uint16_t usPixels[320];
      png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
}
int main(int argc, const char * argv[]) {
    int i, rc;
    uint8_t *pData;
    int iDataSize;
    FILE *ihandle;
    uint8_t *pPalette;
    
    if (argc != 3) {
       printf("Usage: png_demo <infile.png> <outfile.bmp>\n");
       return 0;
    }
    ihandle = fopen(argv[1],"rb"); // open input file
    if (ihandle == NULL)
    {
        fprintf(stderr, "Unable to open file: %s\n", argv[1]);
        return -1; // bad filename passed
    }
    fseek(ihandle, 0L, SEEK_END); // get the file size
    iDataSize = (int)ftell(ihandle);
    fseek(ihandle, 0, SEEK_SET);
    pData = (uint8_t *)malloc(iDataSize);
    fread(pData, 1, iDataSize, ihandle);
    fclose(ihandle);
    
//    for (int j=0; j<10000; j++) {
        rc = png.openRAM(pData, iDataSize, NULL); //PNGDraw);
    if (rc == PNG_SUCCESS) {
        printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
        png.setBuffer((uint8_t *)malloc(png.getBufferSize()));
        rc = png.decode(NULL, 0); //PNG_CHECK_CRC);
        printf("decode returned: %d\n", rc);
        i = 1;
        pPalette = NULL;
        switch (png.getPixelType()) {
            case PNG_PIXEL_INDEXED:
                pPalette = png.getPalette();
                i = 1;
                break;
            case PNG_PIXEL_TRUECOLOR:
                i = 3;
                break;
            case PNG_PIXEL_TRUECOLOR_ALPHA:
                i = 4;
                break;
        }
        SaveBMP((char *)argv[2], png.getBuffer(), pPalette, png.getWidth(), png.getHeight(), i*png.getBpp());
        png.close();
        free(png.getBuffer());
//    } // for j
    }
    return 0;
}
