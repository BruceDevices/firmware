#include "LR11x0.h"

#include "../../utils/Cryptography.h"
#include "LR_common.h"

#include <string.h>

#if !RADIOLIB_EXCLUDE_LR11X0

int16_t LR11x0::cryptoSetKey(uint8_t keyId, const uint8_t* key) {
  RADIOLIB_ASSERT_PTR(key);
  uint8_t buff[1 + RADIOLIB_AES128_KEY_SIZE] = { 0 };
  buff[0] = keyId;
  memcpy(&buff[1], key, RADIOLIB_AES128_KEY_SIZE);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_SET_KEY, false, buff, sizeof(buff)));
}

int16_t LR11x0::cryptoDeriveKey(uint8_t srcKeyId, uint8_t dstKeyId, const uint8_t* key) {
  RADIOLIB_ASSERT_PTR(key);
  uint8_t buff[2 + RADIOLIB_AES128_KEY_SIZE] = { 0 };
  buff[0] = srcKeyId;
  buff[1] = dstKeyId;
  memcpy(&buff[2], key, RADIOLIB_AES128_KEY_SIZE);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_DERIVE_KEY, false, buff, sizeof(buff)));
}

int16_t LR11x0::cryptoProcessJoinAccept(uint8_t decKeyId, uint8_t verKeyId, uint8_t lwVer, const uint8_t* header, const uint8_t* dataIn, size_t len, uint8_t* dataOut) {
  // calculate buffer sizes
  size_t headerLen = 1;
  if(lwVer) {
    headerLen += 11; // LoRaWAN 1.1 header is 11 bytes longer than 1.0
  }
  size_t reqLen = 3*sizeof(uint8_t) + headerLen + len;
  size_t rplLen = sizeof(uint8_t) + len;

  // build buffers
  #if RADIOLIB_STATIC_ONLY
    uint8_t reqBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
    uint8_t rplBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* reqBuff = new uint8_t[reqLen];
    uint8_t* rplBuff = new uint8_t[rplLen];
  #endif
  
  // set the request fields
  reqBuff[0] = decKeyId;
  reqBuff[1] = verKeyId;
  reqBuff[2] = lwVer;
  memcpy(&reqBuff[3], header, headerLen);
  memcpy(&reqBuff[3 + headerLen], dataIn, len);

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_PROCESS_JOIN_ACCEPT, false, rplBuff, rplLen, reqBuff, reqLen);
  #if !RADIOLIB_STATIC_ONLY
    delete[] reqBuff;
  #endif
  if(state != RADIOLIB_ERR_NONE) {
    #if !RADIOLIB_STATIC_ONLY
      delete[] rplBuff;
    #endif
    return(state);
  }

  // check the crypto engine state
  if(rplBuff[0] != RADIOLIB_LR11X0_CRYPTO_STATUS_SUCCESS) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Crypto Engine error: %02x", rplBuff[0]);
    #if !RADIOLIB_STATIC_ONLY
      delete[] rplBuff;
    #endif
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  }

  // pass the data
  memcpy(dataOut, &rplBuff[1], len);
  #if !RADIOLIB_STATIC_ONLY
    delete[] rplBuff;
  #endif
  return(state);
}

int16_t LR11x0::cryptoComputeAesCmac(uint8_t keyId, const uint8_t* data, size_t len, uint32_t* mic) {
  size_t reqLen = sizeof(uint8_t) + len;
  #if RADIOLIB_STATIC_ONLY
    uint8_t reqBuff[sizeof(uint8_t) + RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* reqBuff = new uint8_t[reqLen];
  #endif
  uint8_t rplBuff[5] = { 0 };
  
  reqBuff[0] = keyId;
  memcpy(&reqBuff[1], data, len);

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_COMPUTE_AES_CMAC, false, rplBuff, sizeof(rplBuff), reqBuff, reqLen);
  #if !RADIOLIB_STATIC_ONLY
    delete[] reqBuff;
  #endif

  // check the crypto engine state
  if(rplBuff[0] != RADIOLIB_LR11X0_CRYPTO_STATUS_SUCCESS) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Crypto Engine error: %02x", rplBuff[0]);
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  }

  if(mic) { *mic = ((uint32_t)(rplBuff[1]) << 24) |  ((uint32_t)(rplBuff[2]) << 16) | ((uint32_t)(rplBuff[3]) << 8) | (uint32_t)rplBuff[4]; }
  return(state);
}

int16_t LR11x0::cryptoVerifyAesCmac(uint8_t keyId, uint32_t micExp, const uint8_t* data, size_t len, bool* result) {
   size_t reqLen = sizeof(uint8_t) + sizeof(uint32_t) + len;
  #if RADIOLIB_STATIC_ONLY
    uint8_t reqBuff[sizeof(uint8_t) + sizeof(uint32_t) + RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* reqBuff = new uint8_t[reqLen];
  #endif
  uint8_t rplBuff[1] = { 0 };
  
  reqBuff[0] = keyId;
  reqBuff[1] = (uint8_t)((micExp >> 24) & 0xFF);
  reqBuff[2] = (uint8_t)((micExp >> 16) & 0xFF);
  reqBuff[3] = (uint8_t)((micExp >> 8) & 0xFF);
  reqBuff[4] = (uint8_t)(micExp & 0xFF);
  memcpy(&reqBuff[5], data, len);

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_VERIFY_AES_CMAC, false, rplBuff, sizeof(rplBuff), reqBuff, reqLen);
  #if !RADIOLIB_STATIC_ONLY
    delete[] reqBuff;
  #endif

  // check the crypto engine state
  if(rplBuff[0] != RADIOLIB_LR11X0_CRYPTO_STATUS_SUCCESS) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Crypto Engine error: %02x", rplBuff[0]);
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  }

  if(result) { *result = (rplBuff[0] == RADIOLIB_LR11X0_CRYPTO_STATUS_SUCCESS); }
  return(state);
}

int16_t LR11x0::cryptoAesEncrypt01(uint8_t keyId, const uint8_t* dataIn, size_t len, uint8_t* dataOut) {
  return(this->cryptoCommon(RADIOLIB_LR11X0_CMD_CRYPTO_AES_ENCRYPT_01, keyId, dataIn, len, dataOut));
}

int16_t LR11x0::cryptoAesEncrypt(uint8_t keyId, const uint8_t* dataIn, size_t len, uint8_t* dataOut) {
  return(this->cryptoCommon(RADIOLIB_LR11X0_CMD_CRYPTO_AES_ENCRYPT, keyId, dataIn, len, dataOut));
}

int16_t LR11x0::cryptoAesDecrypt(uint8_t keyId, const uint8_t* dataIn, size_t len, uint8_t* dataOut) {
  return(this->cryptoCommon(RADIOLIB_LR11X0_CMD_CRYPTO_AES_DECRYPT, keyId, dataIn, len, dataOut));
}

int16_t LR11x0::cryptoStoreToFlash(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_STORE_TO_FLASH, true, NULL, 0));
}

int16_t LR11x0::cryptoRestoreFromFlash(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_RESTORE_FROM_FLASH, true, NULL, 0));
}

int16_t LR11x0::cryptoSetParam(uint8_t id, uint32_t value) {
  uint8_t buff[5] = {
    id,
    (uint8_t)((value >> 24) & 0xFF), (uint8_t)((value >> 16) & 0xFF),
    (uint8_t)((value >> 8) & 0xFF), (uint8_t)(value & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_SET_PARAM, true, buff, sizeof(buff)));
}

int16_t LR11x0::cryptoGetParam(uint8_t id, uint32_t* value) {
  uint8_t reqBuff[1] = { id };
  uint8_t rplBuff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_GET_PARAM, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);
  if(value) { *value = ((uint32_t)(rplBuff[0]) << 24) | ((uint32_t)(rplBuff[1]) << 16) | ((uint32_t)(rplBuff[2]) << 8) | (uint32_t)rplBuff[3]; }
  return(state);
}

int16_t LR11x0::cryptoCheckEncryptedFirmwareImage(uint32_t offset, const uint32_t* data, size_t len, bool nonvolatile) {
  // check maximum size
  if(len > (RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->writeCommon(RADIOLIB_LR11X0_CMD_CRYPTO_CHECK_ENCRYPTED_FIRMWARE_IMAGE, offset, data, len, nonvolatile));
}

int16_t LR11x0::cryptoCheckEncryptedFirmwareImageResult(bool* result) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_CHECK_ENCRYPTED_FIRMWARE_IMAGE_RESULT, false, buff, sizeof(buff));

  // pass the replies
  if(result) { *result = (bool)buff[0]; }
  
  return(state);
}

int16_t LR11x0::cryptoCommon(uint16_t cmd, uint8_t keyId, const uint8_t* dataIn, size_t len, uint8_t* dataOut) {
  // build buffers
  #if RADIOLIB_STATIC_ONLY
    uint8_t reqBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
    uint8_t rplBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* reqBuff = new uint8_t[sizeof(uint8_t) + len];
    uint8_t* rplBuff = new uint8_t[sizeof(uint8_t) + len];
  #endif
  
  // set the request fields
  reqBuff[0] = keyId;
  memcpy(&reqBuff[1], dataIn, len);

  int16_t state = this->SPIcommand(cmd, false, rplBuff, sizeof(uint8_t) + len, reqBuff, sizeof(uint8_t) + len);
  #if !RADIOLIB_STATIC_ONLY
    delete[] reqBuff;
  #endif
  if(state != RADIOLIB_ERR_NONE) {
    #if !RADIOLIB_STATIC_ONLY
      delete[] rplBuff;
    #endif
    return(state);
  }

  // check the crypto engine state
  if(rplBuff[0] != RADIOLIB_LR11X0_CRYPTO_STATUS_SUCCESS) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Crypto Engine error: %02x", rplBuff[0]);
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  }

  // pass the data
  memcpy(dataOut, &rplBuff[1], len);
  #if !RADIOLIB_STATIC_ONLY
    delete[] rplBuff;
  #endif
  return(state);
}

#endif
