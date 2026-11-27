#ifndef __NRF_CENSUS_H
#define __NRF_CENSUS_H

// 2.4 GHz census on the nRF24 (task 2.2).
//
// The nRF24L01+ has no promiscuous mode: it hears a packet only when it is
// tuned to that channel AND its address correlator matches the packet's
// syncword. So this is address cycling, not sniffing - it sweeps the 126
// channels against a list of addresses that devices in the room are likely to
// use (the module defaults and the RGB-controller syncwords this firmware
// already knows), counts what answers, and shows the busy channels plus the
// addresses that actually spoke.
//
// What it proves: "something using this address is transmitting on this
// channel, and I heard it N times". What it cannot prove: who it is, or that
// nothing else is there - a device on an address not in the list is invisible,
// and that limit is on the screen, not hidden in a comment.
void nrf_census();

#endif
