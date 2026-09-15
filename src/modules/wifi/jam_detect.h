#pragma once

#if !defined(LITE_VERSION)

// Deauth/disassoc flood ("jamming") detector.
// Watches management-frame deauth rate on the selected channel and raises a
// visual alert when it crosses a user-adjustable threshold.
void jam_detect_setup();

#endif
