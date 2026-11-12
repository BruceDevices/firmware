#ifndef __UART_TERMINAL_H__
#define __UART_TERMINAL_H__
#if !defined(LITE_VERSION)

// Interactive UART terminal: pick a baudrate (presets or custom), reuse or remap the
// bruceConfigPins.uart_bus pins, then talk to the device over a live terminal view.
void uart_terminal_setup();

#endif
#endif
