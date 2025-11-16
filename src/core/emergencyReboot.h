#pragma once

// Tracks long-press on the Back/Esc button and triggers an emergency reboot.
// Call update() frequently (taskInputHandler does this) with the current raw back-button state.
namespace EmergencyReboot {
void update(bool backPressed);
void cancel();
} // namespace EmergencyReboot
