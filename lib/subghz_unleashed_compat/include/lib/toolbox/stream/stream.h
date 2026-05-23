#ifndef __SUBGHZ_COMPAT_STREAM_H__
#define __SUBGHZ_COMPAT_STREAM_H__

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
class Stream;
extern "C" {
#else
typedef struct Stream Stream;
#endif

size_t stream_size(Stream* s);
size_t stream_tell(Stream* s);
bool stream_seek(Stream* s, size_t offset, int origin);
bool stream_read_line(Stream* s, struct FuriString* out);
void stream_clean(Stream* s);
void stream_write_cstring(Stream* s, const char* str);
void stream_write_char(Stream* s, char c);

#ifdef __cplusplus
}
#endif

#endif
