#ifndef __USER_CLASSES_JS_H__
#define __USER_CLASSES_JS_H__

// MUST BE IN THE SAME ORDER AS IN THE mqjs_stdlib.c FILE they cannot be guarded by #ifdef LITE_VERSION
#define JS_CLASS_TIMERS_STATE (JS_CLASS_USER + 0)
#define JS_CLASS_SPRITE (JS_CLASS_USER + 1)
#define JS_CLASS_TEXTVIEWER (JS_CLASS_USER + 2)
#define JS_CLASS_GIF (JS_CLASS_USER + 3)
#define JS_CLASS_BUFFER (JS_CLASS_USER + 4)
/* total number of classes */
#define JS_CLASS_COUNT (JS_CLASS_USER + 5)

#endif
