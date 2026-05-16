#include "SSTV.h"
#if !RADIOLIB_EXCLUDE_SSTV

const SSTVMode_t Scottie1 {
  .visCode = RADIOLIB_SSTV_SCOTTIE_1,
  .width = 320,
  .height = 256,
  .scanPixelLen = 432,
  .numTones = 7,
  .tones = {
    { .type = tone_t::GENERIC,        .len = 1500, .freq = 1500 },
    { .type = tone_t::SCAN_GREEN_Y,   .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 1500, .freq = 1500 },
    { .type = tone_t::SCAN_BLUE_CB,   .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 9000, .freq = 1200 },
    { .type = tone_t::GENERIC,        .len = 1500, .freq = 1500 },
    { .type = tone_t::SCAN_RED_CR,    .len = 0,    .freq = 0    }
  }
};

const SSTVMode_t Scottie2 {
  .visCode = RADIOLIB_SSTV_SCOTTIE_2,
  .width = 320,
  .height = 256,
  .scanPixelLen = 275,
  .numTones = 7,
  .tones = {
    { .type = tone_t::GENERIC,        .len = 1500, .freq = 1500 },
    { .type = tone_t::SCAN_GREEN_Y,   .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 1500, .freq = 1500 },
    { .type = tone_t::SCAN_BLUE_CB,   .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 9000, .freq = 1200 },
    { .type = tone_t::GENERIC,        .len = 1500, .freq = 1500 },
    { .type = tone_t::SCAN_RED_CR,    .len = 0,    .freq = 0    }
  }
};

const SSTVMode_t ScottieDX {
  .visCode = RADIOLIB_SSTV_SCOTTIE_DX,
  .width = 320,
  .height = 256,
  .scanPixelLen = 1080,
  .numTones = 7,
  .tones = {
    { .type = tone_t::GENERIC,        .len = 1500, .freq = 1500 },
    { .type = tone_t::SCAN_GREEN_Y,   .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 1500, .freq = 1500 },
    { .type = tone_t::SCAN_BLUE_CB,   .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 9000, .freq = 1200 },
    { .type = tone_t::GENERIC,        .len = 1500, .freq = 1500 },
    { .type = tone_t::SCAN_RED_CR,    .len = 0,    .freq = 0    }
  }
};

const SSTVMode_t Martin1 {
  .visCode = RADIOLIB_SSTV_MARTIN_1,
  .width = 320,
  .height = 256,
  .scanPixelLen = 458,
  .numTones = 8,
  .tones = {
    { .type = tone_t::GENERIC,        .len = 4862, .freq = 1200 },
    { .type = tone_t::GENERIC,        .len = 572,  .freq = 1500 },
    { .type = tone_t::SCAN_GREEN_Y,   .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 572,  .freq = 1500 },
    { .type = tone_t::SCAN_BLUE_CB,   .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 572,  .freq = 1500 },
    { .type = tone_t::SCAN_RED_CR,    .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 572,  .freq = 1500 }
  }
};

const SSTVMode_t Martin2 {
  .visCode = RADIOLIB_SSTV_MARTIN_2,
  .width = 320,
  .height = 256,
  .scanPixelLen = 229,
  .numTones = 8,
  .tones = {
    { .type = tone_t::GENERIC,        .len = 4862, .freq = 1200 },
    { .type = tone_t::GENERIC,        .len = 572,  .freq = 1500 },
    { .type = tone_t::SCAN_GREEN_Y,   .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 572,  .freq = 1500 },
    { .type = tone_t::SCAN_BLUE_CB,   .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 572,  .freq = 1500 },
    { .type = tone_t::SCAN_RED_CR,    .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 572,  .freq = 1500 }
  }
};

const SSTVMode_t Wrasse {
  .visCode = RADIOLIB_SSTV_WRASSE_SC2_180,
  .width = 320,
  .height = 256,
  .scanPixelLen = 734,
  .numTones = 5,
  .tones = {
    { .type = tone_t::GENERIC,        .len = 5523, .freq = 1200 },
    { .type = tone_t::GENERIC,        .len = 500,  .freq = 1500 },
    { .type = tone_t::SCAN_RED_CR,    .len = 0,    .freq = 0    },
    { .type = tone_t::SCAN_GREEN_Y,   .len = 0,    .freq = 0    },
    { .type = tone_t::SCAN_BLUE_CB,   .len = 0,    .freq = 0    }
  }
};

const SSTVMode_t PasokonP3 {
  .visCode = RADIOLIB_SSTV_PASOKON_P3,
  .width = 640,
  .height = 496,
  .scanPixelLen = 208,
  .numTones = 7,
  .tones = {
    { .type = tone_t::GENERIC,        .len = 5208, .freq = 1200 },
    { .type = tone_t::GENERIC,        .len = 1042, .freq = 1500 },
    { .type = tone_t::SCAN_RED_CR,    .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 1042, .freq = 1500 },
    { .type = tone_t::SCAN_GREEN_Y,   .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 1042, .freq = 1500 },
    { .type = tone_t::SCAN_BLUE_CB,   .len = 0,    .freq = 0    }
  }
};

const SSTVMode_t PasokonP5 {
  .visCode = RADIOLIB_SSTV_PASOKON_P5,
  .width = 640,
  .height = 496,
  .scanPixelLen = 312,
  .numTones = 7,
  .tones = {
    { .type = tone_t::GENERIC,        .len = 7813, .freq = 1200 },
    { .type = tone_t::GENERIC,        .len = 1563, .freq = 1500 },
    { .type = tone_t::SCAN_RED_CR,    .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 1563, .freq = 1500 },
    { .type = tone_t::SCAN_GREEN_Y,   .len = 0,    .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 1563, .freq = 1500 },
    { .type = tone_t::SCAN_BLUE_CB,   .len = 0,    .freq = 0    }
  }
};

const SSTVMode_t PasokonP7 {
  .visCode = RADIOLIB_SSTV_PASOKON_P7,
  .width = 640,
  .height = 496,
  .scanPixelLen = 417,
  .numTones = 7,
  .tones = {
    { .type = tone_t::GENERIC,        .len = 10417, .freq = 1200 },
    { .type = tone_t::GENERIC,        .len = 2083,  .freq = 1500 },
    { .type = tone_t::SCAN_RED_CR,    .len = 0,     .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 2083,  .freq = 1500 },
    { .type = tone_t::SCAN_GREEN_Y,   .len = 0,     .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 2083,  .freq = 1500 },
    { .type = tone_t::SCAN_BLUE_CB,   .len = 0,     .freq = 0    }
  }
};

const SSTVMode_t Robot36 {
  .visCode = RADIOLIB_SSTV_ROBOT_36,
  .width = 320,
  .height = 240,
  .scanPixelLen = 275, // this is the Y-scan length, Cb/Cr are one half
  .numTones = 6,
  .tones = {
    { .type = tone_t::GENERIC,        .len = 9000,  .freq = 1200 },
    { .type = tone_t::GENERIC,        .len = 3000,  .freq = 1500 },
    { .type = tone_t::SCAN_GREEN_Y,   .len = 0,     .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 4500,  .freq = 1500 },
    { .type = tone_t::GENERIC,        .len = 1500,  .freq = 1900 },
    { .type = tone_t::SCAN_BLUE_CB,   .len = 0,     .freq = 0   }, // on even lines, this is the Cr component
  }
};

const SSTVMode_t Robot72 {
  .visCode = RADIOLIB_SSTV_ROBOT_72,
  .width = 320,
  .height = 240,
  .scanPixelLen = 431, // this is the Y-scan length, Cb/Cr are one half
  .numTones = 9,
  .tones = {
    { .type = tone_t::GENERIC,        .len = 9000,  .freq = 1200 },
    { .type = tone_t::GENERIC,        .len = 3000,  .freq = 1500 },
    { .type = tone_t::SCAN_GREEN_Y,   .len = 0,     .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 4500,  .freq = 1500 },
    { .type = tone_t::GENERIC,        .len = 1500,  .freq = 1900 },
    { .type = tone_t::SCAN_RED_CR,    .len = 0,     .freq = 0    },
    { .type = tone_t::GENERIC,        .len = 4500,  .freq = 2300 },
    { .type = tone_t::GENERIC,        .len = 1500,  .freq = 1500 },
    { .type = tone_t::SCAN_BLUE_CB,   .len = 0,     .freq = 0    },
  }
};

SSTVClient::SSTVClient(PhysicalLayer* phy) {
  phyLayer = phy;
  #if !RADIOLIB_EXCLUDE_AFSK
  audioClient = nullptr;
  #endif
}

#if !RADIOLIB_EXCLUDE_AFSK
SSTVClient::SSTVClient(AFSKClient* audio) {
  phyLayer = audio->phyLayer;
  audioClient = audio;
}
#endif

#if !RADIOLIB_EXCLUDE_AFSK
int16_t SSTVClient::begin(const SSTVMode_t& mode) {
  if(audioClient == nullptr) {
    // this initialization method can only be used in AFSK mode
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  return(begin(0, mode));
}
#endif

int16_t SSTVClient::begin(float base, const SSTVMode_t& mode) {
  // save mode
  txMode = mode;

  // calculate 24-bit frequency
  baseFreq = (base * 1000000.0f) / phyLayer->freqStep;

  // configure for direct mode
  return(phyLayer->startDirect());
}

int16_t SSTVClient::setCorrection(float correction) {
  // check if mode is initialized
  if(txMode.visCode == 0) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // apply correction factor to all timings
  txMode.scanPixelLen *= correction;
  for(uint8_t i = 0; i < txMode.numTones; i++) {
    txMode.tones[i].len *= correction;
  }
  return(RADIOLIB_ERR_NONE);
}

void SSTVClient::idle() {
  phyLayer->transmitDirect();
  this->tone(RADIOLIB_SSTV_TONE_LEADER);
}

void SSTVClient::sendHeader() {
  // reset line counter
  lineCount = 0;
  phyLayer->transmitDirect();

  // send the first part of header (leader-break-leader)
  this->tone(RADIOLIB_SSTV_TONE_LEADER, RADIOLIB_SSTV_HEADER_LEADER_LENGTH);
  this->tone(RADIOLIB_SSTV_TONE_BREAK, RADIOLIB_SSTV_HEADER_BREAK_LENGTH);
  this->tone(RADIOLIB_SSTV_TONE_LEADER, RADIOLIB_SSTV_HEADER_LEADER_LENGTH);

  // VIS start bit
  this->tone(RADIOLIB_SSTV_TONE_BREAK, RADIOLIB_SSTV_HEADER_BIT_LENGTH);

  // VIS code
  uint8_t parityCount = 0;
  for(uint8_t mask = 0x01; mask < 0x80; mask <<= 1) {
    if(txMode.visCode & mask) {
      this->tone(RADIOLIB_SSTV_TONE_VIS_1, RADIOLIB_SSTV_HEADER_BIT_LENGTH);
      parityCount++;
    } else {
      this->tone(RADIOLIB_SSTV_TONE_VIS_0, RADIOLIB_SSTV_HEADER_BIT_LENGTH);
    }
  }

  // VIS parity
  if(parityCount % 2 == 0) {
    // even parity
    this->tone(RADIOLIB_SSTV_TONE_VIS_0, RADIOLIB_SSTV_HEADER_BIT_LENGTH);
  } else {
    // odd parity
    this->tone(RADIOLIB_SSTV_TONE_VIS_1, RADIOLIB_SSTV_HEADER_BIT_LENGTH);
  }

  // VIS stop bit
  this->tone(RADIOLIB_SSTV_TONE_BREAK, RADIOLIB_SSTV_HEADER_BIT_LENGTH);
}

void SSTVClient::sendLine(const uint32_t* imgLine) {
  // check first line in Scottie modes
  if((lineCount == 0) && ((txMode.visCode == RADIOLIB_SSTV_SCOTTIE_1) || (txMode.visCode == RADIOLIB_SSTV_SCOTTIE_2) || (txMode.visCode == RADIOLIB_SSTV_SCOTTIE_DX))) {
    // send start sync tone
    this->tone(RADIOLIB_SSTV_TONE_BREAK, 9000);
  }

  // send all tones in sequence
  for(uint8_t i = 0; i < txMode.numTones; i++) {
    if((txMode.tones[i].type == tone_t::GENERIC) && (txMode.tones[i].len > 0)) {
      // Robot36 has different separator tones for even and odd lines
      uint32_t freq = txMode.tones[i].freq;
      if((txMode.visCode == RADIOLIB_SSTV_ROBOT_36) && (i == 3)) {
        freq = (lineCount % 2) ? 2300 : txMode.tones[3].freq;
      }

      // sync/porch tones
      this->tone(freq, txMode.tones[i].len);

    } else {
      // scan lines
      for(uint16_t j = 0; j < txMode.width; j++) {
        uint32_t color = imgLine[j];
        uint32_t len = txMode.scanPixelLen;

        // Robot modes work in YCbCr
        if((txMode.visCode == RADIOLIB_SSTV_ROBOT_36) || (txMode.visCode == RADIOLIB_SSTV_ROBOT_72)) {
          uint8_t r = (color & 0x00FF0000) >> 16;
          uint8_t g = (color & 0x0000FF00) >> 8;
          uint8_t b = (color & 0x000000FF);
          uint8_t y = 16.0 + (0.003906 * ((65.738 * r) + (129.057 * g) + (25.064 * b)));
          uint8_t cb = 128.0 + (0.003906 * ((-37.945 * r) + (-74.494 * g) + (112.439 * b)));
          uint8_t cr = 128.0 + (0.003906 * ((112.439 * r) + (-94.154 * g) + (-18.285 * b)));
          color = ((uint32_t)y << 8);
          if(txMode.visCode == RADIOLIB_SSTV_ROBOT_36) {
            // odd lines carry Cb, even lines carry Cr
            color |= (lineCount % 2) ? cb : cr; 
          } else {
            color |= ((uint32_t)cr << 16) | cb;
          }
          
        }

        switch(txMode.tones[i].type) {
          case(tone_t::SCAN_RED_CR):
            color &= 0x00FF0000;
            color >>= 16;
            if((txMode.visCode == RADIOLIB_SSTV_ROBOT_36) || (txMode.visCode == RADIOLIB_SSTV_ROBOT_72)) {
              len /= 2;
            }
            break;
          case(tone_t::SCAN_GREEN_Y):
            color &= 0x0000FF00;
            color >>= 8;
            break;
          case(tone_t::SCAN_BLUE_CB):
            color &= 0x000000FF;
            if((txMode.visCode == RADIOLIB_SSTV_ROBOT_36) || (txMode.visCode == RADIOLIB_SSTV_ROBOT_72)) {
              len /= 2;
            }
            break;
          case(tone_t::GENERIC):
            break;
        }
        this->tone(RADIOLIB_SSTV_TONE_BRIGHTNESS_MIN + ((float)color * 3.1372549f), len);
      }
    }
  }

  // increment line counter (needed for Robot36 mode)
  lineCount++;
}

uint16_t SSTVClient::getPictureHeight() const {
  return(txMode.height);
}

void SSTVClient::tone(float freq, RadioLibTime_t len) {
  Module* mod = phyLayer->getMod();
  RadioLibTime_t start = mod->hal->micros();
  #if !RADIOLIB_EXCLUDE_AFSK
  if(audioClient != nullptr) {
    audioClient->tone(freq, false);
  } else {
    phyLayer->transmitDirect(baseFreq + (freq / phyLayer->freqStep));
  }
  #else
  phyLayer->transmitDirect(baseFreq + (freq / phyLayer->freqStep));
  #endif
  mod->waitForMicroseconds(start, len);
}

#endif
