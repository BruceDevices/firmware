/*!
 *  @file Adafruit_Si4713.cpp
 *
 *  @mainpage Adafruit Si4713 breakout
 *
 *  @section intro_sec Introduction
 *
 * 	I2C Driver for Si4713 breakout
 *
 * 	This is a library for the Adafruit Si4713 breakout:
 * 	http://www.adafruit.com/products/1958
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *  @section author Author
 *
 *  Limor Fried/Ladyada (Adafruit Industries).
 *
 * 	@section license License
 *
 * 	BSD (see license.txt)
 *
 * 	@section  HISTORY
 *
 *     v1.0 - First release
 */

#include "Adafruit_Si4713.h"

/*!
 *    @brief  Instantiates a new Si4713 class
 *    @param  resetpin
 *            number of pin where reset is connected
 *
 */
Adafruit_Si4713::Adafruit_Si4713(int8_t resetpin) { _rst = resetpin; }

/*!
 *    @brief  Setups the i2c and calls powerUp function.
 *    @param  addr
 *            i2c address
 *    @param  theWire
 *            wire object
 *    @return True if initialization was successful, otherwise false.
 *
 */
bool Adafruit_Si4713::begin(uint8_t addr, TwoWire *theWire) {
  if (i2c_dev)
    delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(addr, theWire);
  if (!i2c_dev->begin())
    return false;

  reset();

  powerUp();

  // check for Si4713
  if (getRev() != 13)
    return false;

  return true;
}

/*!
 *    @brief  Resets the registers to default settings and puts chip in
 * powerdown mode
 */
void Adafruit_Si4713::reset() {
  if (_rst > 0) {
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, HIGH);
    delay(10);
    digitalWrite(_rst, LOW);
    delay(10);
    digitalWrite(_rst, HIGH);
  }
}

/*!
 *    @brief  Set chip property over I2C
 *    @param  property
 *            prooperty that will be set
 *    @param  value
 *            value of property
 */
void Adafruit_Si4713::setProperty(uint16_t property, uint16_t value) {
  _i2ccommand[0] = SI4710_CMD_SET_PROPERTY;
  _i2ccommand[1] = 0;
  _i2ccommand[2] = property >> 8;
  _i2ccommand[3] = property & 0xFF;
  _i2ccommand[4] = value >> 8;
  _i2ccommand[5] = value & 0xFF;
  sendCommand(6);

#ifdef SI4713_CMD_DEBUG
  Serial.print("Set Prop ");
  Serial.print(property);
  Serial.print(" = ");
  Serial.println(value);
#endif
}

/*!
 *    @brief  Send command stored in _i2ccommand to chip.
 *    @param  len
 *            length of command that will be send
 */
void Adafruit_Si4713::sendCommand(uint8_t len) {
  // Send command
  i2c_dev->write(_i2ccommand, len);
  // Wait for status CTS bit
  uint8_t status = 0;
  while (!(status & SI4710_STATUS_CTS))
    i2c_dev->read(&status, 1);
}

/*!
 *    @brief  Tunes to given transmit frequency.
 *    @param  freqKHz
 *            frequency in KHz
 */
void Adafruit_Si4713::tuneFM(uint16_t freqKHz) {
  _i2ccommand[0] = SI4710_CMD_TX_TUNE_FREQ;
  _i2ccommand[1] = 0;
  _i2ccommand[2] = freqKHz >> 8;
  _i2ccommand[3] = freqKHz;
  sendCommand(4);
  while ((getStatus() & 0x81) != 0x81) {
    delay(10);
  }
}

/*!
 *    @brief  Sets the output power level and tunes the antenna capacitor
 *    @param  pwr
 *            power value
 *    @param  antcap
 * 	          antenna capacitor (default to 0)
 */
void Adafruit_Si4713::setTXpower(uint8_t pwr, uint8_t antcap) {
  _i2ccommand[0] = SI4710_CMD_TX_TUNE_POWER;
  _i2ccommand[1] = 0;
  _i2ccommand[2] = 0;
  _i2ccommand[3] = pwr;
  _i2ccommand[4] = antcap;
  sendCommand(5);
}
/*!
 *    @brief  Queries the TX status and input audio signal metrics.
 */
void Adafruit_Si4713::readASQ() {
  _i2ccommand[0] = SI4710_CMD_TX_ASQ_STATUS;
  _i2ccommand[1] = 0x1;
  sendCommand(2);

  uint8_t resp[5];
  i2c_dev->read(resp, 5);
  currASQ = resp[1];
  currInLevel = resp[4];
}

/*!
 *    @brief  Queries the status of a previously sent TX Tune Freq, TX Tune
 * Power, or TX Tune Measure using SI4710_CMD_TX_TUNE_STATUS command.
 */
void Adafruit_Si4713::readTuneStatus() {
  _i2ccommand[0] = SI4710_CMD_TX_TUNE_STATUS;
  _i2ccommand[1] = 0x1; // INTACK
  sendCommand(2);

  uint8_t resp[8];
  i2c_dev->read(resp, 8);
  currFreq = (uint16_t(resp[2]) << 8) | resp[3];
  currdBuV = resp[5];
  currAntCap = resp[6];
  currNoiseLevel = resp[7];
}

/*!
 *    @brief  Measure the received noise level at the specified frequency using
 *            SI4710_CMD_TX_TUNE_MEASURE command.
 *    @param  freq
 *            frequency
 */
void Adafruit_Si4713::readTuneMeasure(uint16_t freq) {
  // check freq is multiple of 50khz
  if (freq % 5 != 0) {
    freq -= (freq % 5);
  }
  // Serial.print("Measuring "); Serial.println(freq);
  _i2ccommand[0] = SI4710_CMD_TX_TUNE_MEASURE;
  _i2ccommand[1] = 0;
  _i2ccommand[2] = freq >> 8;
  _i2ccommand[3] = freq;
  _i2ccommand[4] = 0;

  sendCommand(5);
  while (getStatus() != 0x81)
    delay(10);
}

/*!
 *    @brief  Begin RDS
 *            Sets properties as follows:
 *            SI4713_PROP_TX_AUDIO_DEVIATION: 66.25KHz,
 *            SI4713_PROP_TX_RDS_DEVIATION: 2KHz,
 *            SI4713_PROP_TX_RDS_INTERRUPT_SOURCE: 1,
 *            SI4713_PROP_TX_RDS_PS_MIX: 50% mix (default value),
 *            SI4713_PROP_TX_RDS_PS_MISC: 0x1008,
 *            SI4713_PROP_TX_RDS_PS_REPEAT_COUNT: 3,
 *            SI4713_PROP_TX_RDS_MESSAGE_COUNT: 1,
 *            SI4713_PROP_TX_RDS_PS_AF: 0xE0E0,
 *            SI4713_PROP_TX_RDS_FIFO_SIZE: 0,
 *            SI4713_PROP_TX_COMPONENT_ENABLE: 7
 *    @param  programID
 *            sets SI4713_PROP_TX_RDS_PI to parameter value
 */
void Adafruit_Si4713::beginRDS(uint16_t programID) {
  setProperty(SI4713_PROP_TX_AUDIO_DEVIATION,
              6625);                              // 66.25KHz (default is 68.25)
  setProperty(SI4713_PROP_TX_RDS_DEVIATION, 200); // 2KHz (default)
  setProperty(SI4713_PROP_TX_RDS_INTERRUPT_SOURCE, 0x0001); // RDS IRQ
  setProperty(SI4713_PROP_TX_RDS_PI, programID);      // program identifier
  setProperty(SI4713_PROP_TX_RDS_PS_MIX, 0x03);       // 50% mix (default)
  setProperty(SI4713_PROP_TX_RDS_PS_MISC, 0x1008);    // RDSD0 & RDSMS (default)
  setProperty(SI4713_PROP_TX_RDS_PS_REPEAT_COUNT, 3); // 3 repeats (default)
  setProperty(SI4713_PROP_TX_RDS_MESSAGE_COUNT, 1);   // 1 message (default)
  setProperty(SI4713_PROP_TX_RDS_PS_AF, 0xE0E0);      // no AF (default)
  setProperty(SI4713_PROP_TX_RDS_FIFO_SIZE, 0);       // no FIFO (default)
  setProperty(SI4713_PROP_TX_COMPONENT_ENABLE,
              0x0007); // enable RDS, stereo, tone
}

/*!
 *    @brief  Set up the RDS station string
 *    @param  *s
 *            string to load
 */
void Adafruit_Si4713::setRDSstation(const char *s) {
  uint8_t len = strlen(s);
  uint8_t slots = (len + 3) / 4;

  for (uint8_t i = 0; i < slots; i++) {
    memset(_i2ccommand, ' ', 6); // clear it with ' '
    memcpy(_i2ccommand + 2, s, min(4, (int)strlen(s)));
    s += 4;
    _i2ccommand[6] = 0;
#ifdef SI4713_CMD_DEBUG
    Serial.print("Set slot #");
    Serial.print(i);
    char *slot = (char *)(_i2ccommand + 2);
    Serial.print(" to '");
    Serial.print(slot);
    Serial.println("'");
#endif
    _i2ccommand[0] = SI4710_CMD_TX_RDS_PS;
    _i2ccommand[1] = i; // slot #
    sendCommand(6);
  }
}

/*!
 *    @brief  Queries the status of the RDS Group Buffer and loads new data into
 * buffer.
 *    @param  *s
 *            string to load
 */
void Adafruit_Si4713::setRDSbuffer(const char *s) {
  uint8_t len = strlen(s);
  uint8_t slots = (len + 3) / 4;

  for (uint8_t i = 0; i < slots; i++) {
    memset(_i2ccommand, ' ', 8); // clear it with ' '
    memcpy(_i2ccommand + 4, s, min(4, (int)strlen(s)));
    s += 4;
    _i2ccommand[8] = 0;
#ifdef SI4713_CMD_DEBUG
    Serial.print("Set buff #");
    Serial.print(i);
    char *slot = (char *)(_i2ccommand + 4);
    Serial.print(" to '");
    Serial.print(slot);
    Serial.println("'");
#endif
    _i2ccommand[0] = SI4710_CMD_TX_RDS_BUFF;
    if (i == 0)
      _i2ccommand[1] = 0x06;
    else
      _i2ccommand[1] = 0x04;

    _i2ccommand[2] = 0x20;
    _i2ccommand[3] = i;
    sendCommand(8);
  }
}

/*!
 *    @brief  Read interrupt status bits.
 *    @return status bits
 */
uint8_t Adafruit_Si4713::getStatus() {
  uint8_t resp[1] = {SI4710_CMD_GET_INT_STATUS};
  i2c_dev->write_then_read(resp, 1, resp, 1);
  return resp[0];
}

/*!
 *    @brief  Sends power up command to the breakout, than CTS and GPO2 output
 * is disabled and than enable xtal oscilator. Also It sets properties:
 *            SI4713_PROP_REFCLK_FREQ: 32.768
 *            SI4713_PROP_TX_PREEMPHASIS: 74uS pre-emph (USA standard)
 *            SI4713_PROP_TX_ACOMP_GAIN: max gain
 *            SI4713_PROP_TX_ACOMP_ENABLE: turned on limiter and AGC
 */
void Adafruit_Si4713::powerUp() {
  _i2ccommand[0] = SI4710_CMD_POWER_UP;
  _i2ccommand[1] = 0x12;
  // CTS interrupt disabled
  // GPO2 output disabled
  // Boot normally
  // xtal oscillator ENabled
  // FM transmit
  _i2ccommand[2] = 0x50; // analog input mode
  sendCommand(3);

  // configuration! see page 254
  setProperty(SI4713_PROP_REFCLK_FREQ, 32768); // crystal is 32.768
  setProperty(SI4713_PROP_TX_PREEMPHASIS, 0);  // 74uS pre-emph (USA std)
  setProperty(SI4713_PROP_TX_ACOMP_GAIN, 10);  // max gain?
  // setProperty(SI4713_PROP_TX_ACOMP_ENABLE, 0x02); // turn on limiter, but no
  // dynamic ranging
  setProperty(SI4713_PROP_TX_ACOMP_ENABLE, 0x0); // turn on limiter and AGC
}

/*!
 *    @brief  Get the hardware revision code from the device using
 * SI4710_CMD_GET_REV
 *    @return revision number
 */
uint8_t Adafruit_Si4713::getRev() {
  _i2ccommand[0] = SI4710_CMD_GET_REV;
  _i2ccommand[1] = 0;
  sendCommand(2);

  uint8_t pn, resp[9];
  i2c_dev->read(resp, 9);
  pn = resp[1];

#ifdef SI4713_CMD_DEBUG
  uint8_t fw, patch, cmp, chiprev;
  fw = (uint16_t(resp[2]) << 8) | resp[3];
  patch = (uint16_t(resp[4]) << 8) | resp[5];
  cmp = (uint16_t(resp[6]) << 8) | resp[7];
  chiprev = resp[8];

  Serial.print("Part # Si47");
  Serial.print(pn);
  Serial.print("-");
  Serial.println(fw, HEX);
  Serial.print("Firmware 0x");
  Serial.println(fw, HEX);
  Serial.print("Patch 0x");
  Serial.println(patch, HEX);
  Serial.print("Chip rev ");
  Serial.write(chiprev);
  Serial.println();
#endif

  return pn;
}

/*!
 *    @brief  Configures GP1 / GP2 as output or Hi-Z.
 *    @param  x
 *            bit value
 */
void Adafruit_Si4713::setGPIOctrl(uint8_t x) {
#ifdef SI4713_CMD_DEBUG
  Serial.println("GPIO direction");
#endif
  _i2ccommand[0] = SI4710_CMD_GPO_CTL;
  _i2ccommand[1] = x;
  sendCommand(2);
}

/*!
 *    @brief  Sets GP1 / GP2 output level (low or high).
 *    @param  x
 *            bit value
 */
void Adafruit_Si4713::setGPIO(uint8_t x) {
#ifdef SI4713_CMD_DEBUG
  Serial.println("GPIO set");
#endif
  _i2ccommand[0] = SI4710_CMD_GPO_SET;
  _i2ccommand[1] = x;
  sendCommand(2);
}
