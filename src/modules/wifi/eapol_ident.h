#ifndef __EAPOL_IDENT_H__
#define __EAPOL_IDENT_H__

#include <Arduino.h>

// Task 5, the 802.1X half.
//
// A PEAP/MSCHAPv2 attack needs a TLS server to open the tunnel, and the
// challenge/response only exists inside that tunnel - so an honest passive pass
// can only take what travels in the clear. That is a lot: EAPOL-Start frames and
// EAP Response/Identity carry the *outer* identity (DOMAIN\user or user@realm)
// and the requested method (PEAP, TTLS, MD5, TLS) for every client that tries to
// join an enterprise network. This page/listener collects exactly that and says
// so - it is a credential *harvest*, not a cracked password.
namespace EapolIdent {

// Listen for `seconds` on 1-13 and return the report (identities, BSSIDs,
// requested methods). Honest empty report when no 802.1X client is around.
String harvest(int seconds);

// Panel wrapper: listen 20 s and show the report.
void menu();

} // namespace EapolIdent

#endif
