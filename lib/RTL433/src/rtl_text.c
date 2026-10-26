#include "rtl_text.h"
#include <string.h>
unsigned rtl_text_page(const char *s, unsigned cols, unsigned rows,
                       unsigned offset, char *out, size_t stride) {
  if (!s || !cols || (out && stride < cols + 1)) return 0;
  if (out) memset(out, 0, rows * stride);
  unsigned line = 0, col = 0;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    if (*p == '\n') { line++; col = 0; continue; }
    if (col == cols) { line++; col = 0; }
    if (out && line >= offset && line - offset < rows)
      out[(line - offset) * stride + col] = *p >= 32 && *p < 127 ? *p : '?';
    col++;
  }
  return *s ? line + 1 : 0;
}
