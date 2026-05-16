//
// PNG decoder benchmark
//
// Runs images through the PNG decoder without displaying the pixels
// to measure the execution time on various Arduino boards
//
#include <PNGdec.h>
#include "octocat_4bpp.h"
#include "octocat_8bpp.h"
#include "octocat_32bpp.h"

PNG png; // statically allocate the PNG structure (about 50K of RAM)

// simple private structure to pass a boolean value to the PNGDRAW callback
typedef struct myprivate
{
  bool bConvert;
} PRIVATE;

int PNGDraw(PNGDRAW *pDraw)
{
PRIVATE *pPriv = (PRIVATE *)pDraw->pUser;
uint16_t usPixels[320];

  if (pPriv->bConvert)
     png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff); // don't do alpha color blending
  return 1;
} /* PNGDraw() */

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Starting benchmark...");
} /* setup() */

void loop() {
long lTime;
char szTemp[256];
int rc;
PRIVATE priv;

// 32-bpp image
    rc = png.openFLASH((uint8_t *)octocat_32bpp, sizeof(octocat_32bpp), PNGDraw);
    if (rc == PNG_SUCCESS) {
        Serial.println("Successfully opened octocat_32bpp.png");
        priv.bConvert = false;
        lTime = micros();
        rc = png.decode((void *)&priv, PNG_FAST_PALETTE);
        if (rc == PNG_SUCCESS) {
          lTime = micros() - lTime;
          sprintf(szTemp, "Decode time for native pixels = %d us\n", (int)lTime);
          Serial.print(szTemp);
        } else {
          Serial.print("Decode error: "); Serial.println(rc, DEC);
        }
        priv.bConvert = true;
        lTime = micros();
        rc = png.decode((void *)&priv, PNG_FAST_PALETTE);
        lTime = micros() - lTime;
        sprintf(szTemp, "Decode time for RGB565 pixels = %d us\n", (int)lTime);
        Serial.print(szTemp); 
        png.close(); // not needed for memory->memory decode
    }

// 8-bpp image
    rc = png.openFLASH((uint8_t *)octocat_8bpp, sizeof(octocat_8bpp), PNGDraw);
    if (rc == PNG_SUCCESS) {
        Serial.println("Successfully opened octocat_8bpp.png");
        priv.bConvert = false;
        lTime = micros();
        rc = png.decode((void *)&priv, PNG_FAST_PALETTE);
        lTime = micros() - lTime;
        sprintf(szTemp, "Decode time for native pixels = %d us\n", (int)lTime);
        Serial.print(szTemp);
        priv.bConvert = true;
        lTime = micros();
        rc = png.decode((void *)&priv, PNG_FAST_PALETTE);
        lTime = micros() - lTime;
        sprintf(szTemp, "Decode time for RGB565 pixels = %d us\n", (int)lTime);
        Serial.print(szTemp);
        png.close(); // not needed for memory->memory decode
    }
// 4-bpp image
    rc = png.openFLASH((uint8_t *)octocat_4bpp, sizeof(octocat_4bpp), PNGDraw);
    if (rc == PNG_SUCCESS) {
        Serial.println("Successfully opened octocat_4bpp.png");
        priv.bConvert = false;
        lTime = micros();
        rc = png.decode((void *)&priv, PNG_FAST_PALETTE);
        lTime = micros() - lTime;
        sprintf(szTemp, "Decode time for native pixels = %d us\n", (int)lTime);
        Serial.print(szTemp);
        priv.bConvert = true;
        lTime = micros();
        rc = png.decode((void *)&priv, PNG_FAST_PALETTE);
        lTime = micros() - lTime;
        sprintf(szTemp, "Decode time for RGB565 pixels = %d us\n", (int)lTime);
        Serial.print(szTemp); 
        png.close(); // not needed for memory->memory decode
    }
  delay(5000);
} /* loop() */
