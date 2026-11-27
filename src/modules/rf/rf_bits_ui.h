#ifndef __RF_BITS_UI_H__
#define __RF_BITS_UI_H__

// Task 2: the on-screen half of the sub-GHz bit editor. The CLI (`rfbits`) and
// this page drive the same RfBits state, so an edit made with the encoder is the
// one `rfbits show` reports and the one `rfbits tx` sends.
void rfBitsEditor();

#endif
