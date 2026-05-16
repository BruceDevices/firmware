// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#pragma once

#include "Arduino.h"

bool checkBasicAuthentication(const char *header, const char *username, const char *password);

bool checkDigestAuthentication(
  const char *header, const char *method, const char *username, const char *password, const char *realm, bool passwordIsHash, const char *nonce,
  const char *opaque, const char *uri
);

// for storing hashed versions on the device that can be authenticated against
String generateDigestHash(const char *username, const char *password, const char *realm);

String generateBasicHash(const char *username, const char *password);

String genRandomMD5();
