// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#include <ESPAsyncWebServer.h>

/**
 * @brief Sends a file from the filesystem to the client, with optional gzip compression and ETag-based caching.
 *
 * This method serves files over HTTP from the provided filesystem. If a compressed version of the file
 * (with a `.gz` extension) exists and uncompressed version does not exist, it serves the compressed file.
 * It also handles ETag caching using the CRC32 value from the gzip trailer, responding with `304 Not Modified`
 * if the client's `If-None-Match` header matches the generated ETag.
 *
 * @param fs Reference to the filesystem (SPIFFS, LittleFS, etc.).
 * @param path Path to the file to be served.
 * @param contentType Optional MIME type of the file to be sent.
 *                    If contentType is "" it will be obtained from the file extension
 * @param download If true, forces the file to be sent as a download.
 * @param callback Optional template processor for dynamic content generation.
 *                 Templates will not be processed in compressed files.
 *
 * @note If neither the file nor its compressed version exists, responds with `404 Not Found`.
 */
void AsyncWebServerRequest::send(FS &fs, const String &path, const char *contentType, bool download, AwsTemplateProcessor callback) {
  // Check uncompressed file first
  if (fs.exists(path)) {
    send(beginResponse(fs, path, contentType, download, callback));
    return;
  }

  // Handle compressed version
  const String gzPath = path + asyncsrv::T__gz;
  File gzFile = fs.open(gzPath, fs::FileOpenMode::read);

  // ETag validation
  if (this->hasHeader(asyncsrv::T_INM)) {
    // Generate server ETag from CRC in gzip trailer
    char serverETag[11];
    if (!_getEtag(gzFile, serverETag)) {
      // Compressed file not found or invalid
      send(404);
      gzFile.close();
      return;
    }

    // Compare with client's ETag
    const AsyncWebHeader *inmHeader = this->getHeader(asyncsrv::T_INM);
    if (inmHeader && inmHeader->value() == serverETag) {
      gzFile.close();
      this->send(304);  // Not Modified
      return;
    }
  }

  // Send compressed file response
  gzFile.close();
  send(beginResponse(fs, path, contentType, download, callback));
}

/**
 * @brief Generates an ETag string (enclosed into quotes) from the CRC32 trailer of a GZIP file.
 *
 * This function reads the CRC32 checksum (4 bytes) located at the end of a GZIP-compressed file
 * and converts it into an 8-character hexadecimal ETag string (enclosed in double quotes and null-terminated).
 * Double quotes for ETag value are required by RFC9110 section 8.8.3.
 *
 * @param gzFile  Opened file handle pointing to the GZIP file.
 * @param eTag    Output buffer to store the generated ETag.
 *                Must be pre-allocated with at least 11 bytes (8 for hex digits + 2 for quotes + 1 for null terminator).
 *
 * @return true if the ETag was successfully generated, false otherwise (e.g., file too short or seek failed).
 */
bool AsyncWebServerRequest::_getEtag(File gzFile, char *etag) {
  static constexpr char hexChars[] = "0123456789ABCDEF";

  if (!gzFile.seek(gzFile.size() - 8)) {
    return false;
  }

  uint32_t crc;
  gzFile.read(reinterpret_cast<uint8_t *>(&crc), sizeof(crc));

  etag[0] = '"';
  etag[1] = hexChars[(crc >> 4) & 0x0F];
  etag[2] = hexChars[crc & 0x0F];
  etag[3] = hexChars[(crc >> 12) & 0x0F];
  etag[4] = hexChars[(crc >> 8) & 0x0F];
  etag[5] = hexChars[(crc >> 20) & 0x0F];
  etag[6] = hexChars[(crc >> 16) & 0x0F];
  etag[7] = hexChars[(crc >> 28)];
  etag[8] = hexChars[(crc >> 24) & 0x0F];
  etag[9] = '"';
  etag[10] = '\0';

  return true;
}
