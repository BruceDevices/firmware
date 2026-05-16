#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#define console_printf(_fmt, ...) ::printf(_fmt, ##__VA_ARGS__)
#else
#define console_printf(_fmt, ...) printf(_fmt, ##__VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CONSOLE_H__ */