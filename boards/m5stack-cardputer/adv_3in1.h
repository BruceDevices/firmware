#pragma once

#ifdef CARDPUTER_ADV_3IN1
bool cardputerAdvEnterNrf();
bool cardputerAdvLeaveNrf();
bool cardputerAdvNrfActive();
bool cardputerAdvKeyboardRecoveryPending();
void cardputerAdvLockInput();
void cardputerAdvUnlockInput();
#endif
