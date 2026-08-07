#ifndef __CRASH_DIAG_H__
#define __CRASH_DIAG_H__

// If the previous boot ended in a panic/watchdog/brownout, shows the crash
// details (panic reason, crashing task, PC, backtrace) on the display for a
// few seconds. The details come from the ESP-IDF core dump (ELF format) which
// is automatically written to the "coredump" flash partition by the panic
// handler on every crash/abort/watchdog reset; the record is consumed
// (erased) once shown. Safe to call once per boot, after the display is ready.
void crashDiagShow();

#endif
