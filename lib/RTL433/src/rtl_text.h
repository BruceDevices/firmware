#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* The same wrapping rule counts and renders lines, including exact-width
 * lines followed by a newline. Output is rows * stride bytes. */
unsigned rtl_text_page(const char *s, unsigned cols, unsigned rows,
                       unsigned offset, char *out, size_t stride);
#ifdef __cplusplus
}
#endif
