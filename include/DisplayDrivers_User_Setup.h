// Configuration hook for the DisplayDrivers library.
//
// The library's own src/DisplayDrivers_Setup.h reads this file first (it is
// found through `build_flags = -I include`), before anything is decided, so
// whatever lands here reaches every translation unit — including the library's
// own .cpp files, which never see macros defined in the project's sources.
// The name matters: a DisplayDrivers_Setup.h here would be shadowed by the
// library's copy of that name, which sits earlier on the include path.
//
// Everything that describes a panel lives in boards/<board>/<env>.ini in
// the library's own vocabulary (USE_*, TFT_DATABUS_N, TFT_DISPLAY_DRIVER_N,
// TFT_* pins). This file only carries over the project-wide defaults that the
// boards rely on but never spell out.
#ifndef _BRUCE_DISPLAYDRIVERS_USER_SETUP_H
#define _BRUCE_DISPLAYDRIVERS_USER_SETUP_H

// TFT_WIDTH / TFT_HEIGHT / TFT_MISO / ROTATION and the CYD pin fallbacks. The
// backends need them as much as the Bruce's own sources do.
#include "precompiler_flags.h"

// Bruce calls the panel's mounting rotation ROTATION (it is also the
// default the user can change at runtime); DisplayDrivers calls it
// TFT_ROTATION. Same number, so bridge it here instead of duplicating it in
// forty board files.
#ifndef TFT_ROTATION
#define TFT_ROTATION ROTATION
#endif

#endif // _BRUCE_DISPLAYDRIVERS_USER_SETUP_H
