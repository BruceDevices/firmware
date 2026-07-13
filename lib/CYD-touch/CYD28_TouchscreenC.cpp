/* Touchscreen library for 2432W328 Capacitive Touch Controller Chip
 * Supported boards
 *  - 2432W328C
 */

#include "CYD28_TouchscreenC.h"

static CYD28_TouchC *isrPinptr;

bool CYD28_TouchC::begin(void)
{


    auto _sda = CYD28_TouchC_SDA;
    auto _scl = CYD28_TouchC_SCL;
    auto _rst = CYD28_TouchC_RST;
    auto _int = CYD28_TouchC_INT;
    // Initialize I2C
    if (_sda != -1 && _scl != -1)
    {
        _wire->begin(_sda, _scl);
    }
    else
    {
        _wire->begin();
    }

    // Int Pin Configuration
    if (_int != -1)
    {
        pinMode(_int, OUTPUT);
        digitalWrite(_int, HIGH);
        delay(1);
        digitalWrite(_int, LOW); 
        delay(1);
    }

    // Reset Pin Configuration
    if (_rst != -1)
    {
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, LOW);
        delay(10);
        digitalWrite(_rst, HIGH);
        delay(300);
    }

    // Initialize Touch
    i2c_write(0xFE, 0XFF);
    isrPinptr = this;
    return true;
}

bool CYD28_TouchC::touched()
{
    bool FingerIndex = false;
    FingerIndex = (bool)i2c_read(0x02);

    return FingerIndex;
}

CYD28_TS_Point CYD28_TouchC::getPointScaled()
{
    uint8_t data[4];
    uint16_t x, y;
    i2c_read_continuous(0x03,data,4);
    x = ((data[0] & 0x0f) << 8) | data[1];
    y = ((data[2] & 0x0f) << 8) | data[3];
    return convertRawXY(x, y);
}

uint8_t CYD28_TouchC::i2c_read(uint8_t addr)
{
    uint8_t rdData;
    uint8_t rdDataCount;
    do
    {
        _wire->beginTransmission(CYD28_I2C_ADDR);
        _wire->write(addr);
        _wire->endTransmission(false); // Restart
        rdDataCount = _wire->requestFrom(CYD28_I2C_ADDR, 1);
    } while (rdDataCount == 0);
    while (_wire->available())
    {
        rdData = _wire->read();
    }
    return rdData;
}

uint8_t CYD28_TouchC::i2c_read_continuous(uint8_t addr, uint8_t *data, uint32_t length)
{
  _wire->beginTransmission(CYD28_I2C_ADDR);
  _wire->write(addr);
  if ( _wire->endTransmission(true))return -1;
  _wire->requestFrom(CYD28_I2C_ADDR, length);
  for (int i = 0; i < length; i++) {
    *data++ = _wire->read();
  }
  return 0;
}

void CYD28_TouchC::i2c_write(uint8_t addr, uint8_t data)
{
    _wire->beginTransmission(CYD28_I2C_ADDR);
    _wire->write(addr);
    _wire->write(data);
    _wire->endTransmission();
}

CYD28_TS_Point CYD28_TouchC::convertRawXY(int16_t x, int16_t y) {
    int16_t convertX = y;
    int16_t convertY = sizeX_px - 1 - x;
    return CYD28_TS_Point(convertX, convertY, 0);
}