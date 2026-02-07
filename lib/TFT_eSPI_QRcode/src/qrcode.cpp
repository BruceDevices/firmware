#include <Arduino.h>
#include "qrcode.h"
#include "qrencode.h"

int offsetsX;
int offsetsY;
int screenwidth;
int screenheight;
int multiply = 2;

QRcode::QRcode(tft_display *tft)
{
  this->tft = tft;
}

void QRcode::init()
{
  screenwidth = tft->width();
  screenheight = tft->height();
  int min = screenwidth;
  if (screenheight < screenwidth)
    min = screenheight;
  multiply = min / WD;
  offsetsX = (screenwidth - (WD * multiply)) / 2;
  offsetsY = (screenheight - (WD * multiply)) / 2;
}

void QRcode::render(int x, int y, int color)
{
  x = (x * multiply) + offsetsX;
  y = (y * multiply) + offsetsY;
  if (color == 1)
  {
    tft->drawPixel(x, y, TFT_BLACK);
    if (multiply > 1)
    {
      tft->fillRect(x, y, multiply, multiply, TFT_BLACK);
    }
  }
  else
  {
    tft->drawPixel(x, y, TFT_WHITE);
    if (multiply > 1)
    {
      tft->fillRect(x, y, multiply, multiply, TFT_WHITE);
    }
  }
}

void QRcode::create(String message)
{
  // create QR code
  tft->fillScreen(TFT_WHITE);
  message.toCharArray((char *)strinbuf, 260);
  qrencode();
  // print QR Code
  for (byte x = 0; x < WD; x += 2)
  {
    for (byte y = 0; y < WD; y++)
    {
      if (QRBIT(x, y) && QRBIT((x + 1), y))
      {
        // black square on top of black square
        render(x, y, 1);
        render((x + 1), y, 1);
      }
      if (!QRBIT(x, y) && QRBIT((x + 1), y))
      {
        // white square on top of black square
        render(x, y, 0);
        render((x + 1), y, 1);
      }
      if (QRBIT(x, y) && !QRBIT((x + 1), y))
      {
        // black square on top of white square
        render(x, y, 1);
        render((x + 1), y, 0);
      }
      if (!QRBIT(x, y) && !QRBIT((x + 1), y))
      {
        // white square on top of white square
        render(x, y, 0);
        render((x + 1), y, 0);
      }
    }
  }
}
