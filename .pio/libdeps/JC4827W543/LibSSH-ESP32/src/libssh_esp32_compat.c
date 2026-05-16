// ESP32 libssh port.
// libssh_esp32_compat.c
//
// Ewan Parker, created 18th April 2020.
// Missing implementations needed to link libssh with ESP32.
//
// Copyright (C) 2020–2026 Ewan Parker.

#include "libssh_esp32_compat.h"
#include "esp_idf_version.h"

#ifndef LIBSSH_ESP32_COMPAT_USERNAME
#define LIBSSH_ESP32_COMPAT_USERNAME "root"
#endif
#ifndef LIBSSH_ESP32_COMPAT_UID
#define LIBSSH_ESP32_COMPAT_UID 0
#endif
#ifndef LIBSSH_ESP32_COMPAT_GID
#define LIBSSH_ESP32_COMPAT_GID 0
#endif
#ifndef LIBSSH_ESP32_COMPAT_HOMEDIR
#define LIBSSH_ESP32_COMPAT_HOMEDIR "/spiffs"
#endif

#ifndef LIBSSH_ESP32_COMPAT_HOSTNAME
#define LIBSSH_ESP32_COMPAT_HOSTNAME "esp32"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

__attribute__((weak))
pid_t waitpid(pid_t pid, int *wstatus, int options)
{ return 0; }

__attribute__((weak))
uid_t getuid()
{ return LIBSSH_ESP32_COMPAT_UID; }

static struct passwd p =
  { LIBSSH_ESP32_COMPAT_USERNAME, /* password: */"", LIBSSH_ESP32_COMPAT_UID,
    LIBSSH_ESP32_COMPAT_GID, /* comment: */"", /* gecos: */"",
    LIBSSH_ESP32_COMPAT_HOMEDIR, /* shell: */"" };

__attribute__((weak))
int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf,
               size_t buflen, struct passwd **result)
{
  *result = &p;
  memcpy(pwd, &p, sizeof p);
  return 0; // success
}

__attribute__((weak))
struct passwd *getpwnam (const char *name)
{ return &p; }

__attribute__((weak))
#if ESP_IDF_VERSION_MAJOR >= 4
int gethostname(char *name, size_t len)
#else
int gethostname(char *name, int len)
#endif
{
  strncpy(name, LIBSSH_ESP32_COMPAT_HOSTNAME, len);
  return 0;
}
