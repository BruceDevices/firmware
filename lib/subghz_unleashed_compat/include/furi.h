#ifndef __SUBGHZ_COMPAT_FURI_H__
#define __SUBGHZ_COMPAT_FURI_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) ((void)(x))
#define COUNT_OF(x) (sizeof(x) / sizeof((x)[0]))

#define FURI_LOG_E(tag, fmt, ...) ((void)0)
#define FURI_LOG_W(tag, fmt, ...) ((void)0)
#define FURI_LOG_I(tag, fmt, ...) ((void)0)
#define FURI_LOG_D(tag, fmt, ...) ((void)0)

#define furi_check(x)                                                                                             \
    do {                                                                                                          \
        if(!(x)) {                                                                                                \
            abort();                                                                                              \
        }                                                                                                         \
    } while(0)

#define furi_check_ret(x, retv)                                                                                   \
    do {                                                                                                          \
        if(!(x)) {                                                                                                \
            return (retv);                                                                                        \
        }                                                                                                         \
    } while(0)

#define furi_assert(x)                                                                                             \
    do {                                                                                                          \
        if(!(x)) {                                                                                                \
            abort();                                                                                              \
        }                                                                                                         \
    } while(0)

typedef struct FuriString FuriString;

#define RECORD_STORAGE "storage"

#ifdef __cplusplus
extern "C" {
#endif

FuriString* furi_string_alloc(void);
FuriString* furi_string_alloc_set(const char* cstr);
void furi_string_free(FuriString* str);
void furi_string_set(FuriString* str, const char* cstr);
void furi_string_set_n(FuriString* out, const FuriString* in, size_t pos, size_t len);
void furi_string_cat(FuriString* str, const char* cstr);
void furi_string_cat_printf(FuriString* str, const char* fmt, ...);
void furi_string_printf(FuriString* str, const char* fmt, ...);
const char* furi_string_get_cstr(const FuriString* str);
size_t furi_string_size(const FuriString* str);
void furi_string_trim(FuriString* str);

void furi_delay_ms(uint32_t ms);
void furi_delay_tick(uint32_t ticks);
void furi_crash_message(const char* message);
float roundf(float x);

void* furi_record_open(const char* record);
void furi_record_close(const char* record);

#ifdef __cplusplus
}
#endif

#define furi_crash(...) furi_crash_message(NULL)

#endif
