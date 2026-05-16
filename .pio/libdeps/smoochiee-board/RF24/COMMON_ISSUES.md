# Common Issues

@tableofcontents

<!-- markdownlint-disable MD031 -->

## Simple Troubleshooting

When troubleshooting, users can start with the included gettingStarted
sketch as it contains all the info needed to troubleshoot an RF24 radio.

1. If `radio.begin()` returns false, the MCU and radio are not "talking"
successfully with each other. Verify pin connections and wiring.
2. If `radio.begin()` returns true, but the devices are not communicating,
users can  uncomment the lines `printf_begin()` & `radio.printDetails()` and
   view the settings. They should appear as the following:
   
   ```
   SPI Speedz	    = 10 Mhz
   STATUS		    = 0x0e RX_DR=0 TX_DS=0 MAX_RT=0 RX_P_NO=7 TX_FULL=0
   RX_ADDR_P0-1	    = 0x65646f4e31 0x65646f4e32
   RX_ADDR_P2-5	    = 0x33 0xce 0x3e 0xe3
   TX_ADDR		    = 0x65646f4e31
   RX_PW_P0-6	    = 0x04 0x04 0x04 0x04 0x04 0x04
   EN_AA		    = 0x3f
   EN_RXADDR	    = 0x02
   RF_CH		    = 0x4c
   RF_SETUP	        = 0x03
   CONFIG		    = 0x0f
   DYNPD/FEATURE	= 0x00 0x00
   Data Rate	    = 1 MBPS
   Model		    = nRF24L01+
   CRC Length	    = 16 bits
   PA Power	        = PA_LOW
   ARC		        = 0
   ```
   If the settings do not appear as above, troubleshoot wiring, pin
   connections, etc.
   
3. If both of the above check out, the problem is likely the CE pin is wired wrong, or 
even hardware issues (bad radios etc.) See the following.
   

## Settings that must match

Before you report undesirable behavior, please make sure that the
following RF24 configurations match on both receiving and transmitting
nRF24L01 transceivers:

1. `RF24::setAddressWidth()`
2. `RF24::setChannel()`
3. `RF24::setDataRate()`
4. `RF24::setAutoAck()`
5. `RF24::enableDynamicPayloads()` or `RF24::disableDynamicPayloads()`
6. `RF24::enableAckPayload()` or `RF24::disableAckPayload()` (requires auto-ack and
   dynamic payloads features)
7. `RF24::setPayloadSize()` (only if the dynamic payloads feature is disabled -- it is disabled by default)
8. `RF24::setCRCLength()` or `RF24::disableCRC()` (the auto-ack feature
   automatically enables CRC because it is required)

Also, it helps to think of an address as a path (a commonly shared route)
instead of an identifying device destination. This means that addresses
have to match for a payload to travel from one transceiver to another.
However, the pipe numbers assigned with the matching address do not have
to match. You can think of pipes as parking spots for the packets, while
all packets' payloads live in a TX or RX FIFO buffer. Remember that the
TX FIFO buffers and the RX FIFO buffers both have a maximum occupancy of
3 payloads (regardless of the maximum 32-byte payload size).

## Prohibited usage of write*() in Interrupt Service Routine callbacks

Because the RF24 library uses `millis()` to implement a timeout and `delay()` for mandatory wait times, the following functions cannot be used within an ISR callback method:

- `RF24::write()`
- `RF24::writeBlocking()`
- `RF24::writeFast()`
- `RF24::startWrite()`
- `RF24::txStandBy()` & `RF24::txStandBy(uint32_t, bool)`
- `RF24::powerUp()`
- `RF24::startListening()`
- `RF24::stopListening()`

@see The note in the documentation for `RF24::available()`.

More info about why you can't call `millis()` (or `delay()`) from an ISR callback function is available at [the Arduino docs](https://www.google.com/url?sa=t&source=web&rct=j&url=https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/&ved=2ahUKEwjMhtSRl5jzAhVUsp4KHWIPCrIQFnoECAoQAQ&usg=AOvVaw1X9H0058Nz7Hck91VIC3bD).

## Which write*() function do I use?

**Standard:**

`RF24::write()`: The standard write function, this is the most commonly used way to send data over radio link. This function will block until data is sent successfully. This means that if Auto-Ack is enabled, the radio will write the packet and wait for a response from the receiving radio. If Auto-Ack is disabled, the function will return sooner, as it will not wait for a response from the receiving radio.

`RF24::startWrite()`: Can be used similar to the standard `RF24::write()` function, but it will not block. Useful for writing data outside of an interrupt routine, triggering that interrupt. Contains a `delayMicroseconds()` call for faster MCUs/devices to ensure the CE pin is toggled for a full 10us.

**Advanced: (requires calling txStandBy())**

`RF24::writeFast()`: Used for high-speed streaming of data. This function can be used to transmit data by simply placing data in the 3-layer FIFO buffers if room is available, or blocking until available. The function will return after a packet is placed in the buffer, or when a packet fails to transmit, in which case the buffers are cleared.

`RF24::writeBlocking()`: Not commonly used, this function will first check the 3-layer FIFO for available space, then block until a timeout period is met if packets are failing, or return once there is room in the FIFO buffer and a packet is placed there.

Interrupt Safe Functions:

`RF24::startFastWrite()`: Can be used to write data and return immediately, without going into standBy mode. Can be used to transmit data at high speeds using interrupts, but will easily overflow the FIFO buffer if attempting to send data faster than the radio will process it.

Again, some of the other functions can technically be placed inside interrupt routines, but rely on millis() for timeouts etc. and this functionality will not typically work within an interrupt routine. Advanced users can comment out `FAILURE_HANDLING` in `RF24_config.h` to disable some of this functionality on non-linux devices to make functions like `RF24::txStandBy()` interrupt-safe.

## Here are the most common issues and their solutions

### write() always returns true after setAutoAck(false)

Don't disabled the auto-ack feature. RF24::write() has no reason to doubt
that the payload was delivered if the auto-ack feature is disabled. We
recommend you read the docs about RF24::setAutoAck() before disabling the
auto-ack feature.

### write() returns false when the payload was received

If the settings match on both endpoint transceivers, then this can only
mean that the receiving nRF24L01 failed to send an acknowledgement (ACK)
packet back to the transmitting nRF24L01. Usually this is due to
instability (electric noise) in the power lines (VCC and GND) going to
the receiving nRF24L01.

If you're not receiving ACK packets correctly/reliably on data rates
lower than 2MBPS, try adding a big capacitor close to the module/chip.
Example issues: [#264](https://github.com/nRF24/RF24/issues/264)
[#211](https://github.com/nRF24/RF24/issues/211).

For reliability, please use Electrolytic or Tantalum capacitors. Ceramic
capacitors may not be good enough (depending on the manufacturing source).

### printDetails() outputs the unexpected value(s)

This is likely due to the SPI speed being set up to 10MHz by default. We recommend:

1. Make sure the wires are not loose, and try to avoid using long wires.
2. If the previous point does not help, then try lowering the SPI speed like so
   ```cpp
   RF24 radio(7, 8, 4000000); // set SPI speed to 4MHz instead of default 10MHz
   ```

In the RF24 library's beginnings, the default value was (prior to 2014) set to 4MHz.

### Payloads received/sent are inaccurate

If lowering the SPI speed (as noted above) does not fix this problem, then it is likely data misalignment.

Inaccurate payloads can also happen because of differences in how certain systems store data in memory. The easiest way to ensure your data aligns between devices is to specify the size of variables in code. For example, use `uint32_t` instead of `unsigned long`, and use `__attribute__((__packed__))` for data structures if you don't align your data manually per the linked blog posts (see below).

Newer users can use the `sizeof()` function to verify the size of different variables or data structures as well.

See [GNU libc doc about integers](https://www.gnu.org/software/libc/manual/html_node/Integers.html) and [TMRh20's Blog Post](https://tmrh20.blogspot.com/2020/09/transferring-data-between-systems-using.html) for more info.

### My PA/LNA module fails to transmit

You may find variants of the nRF24L01 transceiver that are marketed as “nRF24L01+PA+LNA”. These modules are distinct in the fact that they come with a detachable (SMA-type) antenna. They employ separate RFX24C01 IC with the antenna for enhanced Power Amplification (PA) and Low Noise Amplification (LNA) features. While they boast greater range with the same functionality, they are subject to a couple lesser known (and lesser advertised) drawbacks:

1. Stronger power source. Below is a chart of advertised current requirements that many MCU boards’ 3V regulators may not be able to provide (after supplying power to internal components).
   | Specification | Value |
   |:-------------:|:-----:|
   | Emission mode current(peak) | 115 mA |
   | Receive Mode current(peak)  | 45 mA  |
   | Power-down mode current     | 4.2 µA |

2. Needs shielding from electromagnetic interference. Shielding usually works best when it has a path to ground (GND pin), but this connection to the GND pin is not required. It is important that the shielding does not touch any current carrying parts.
   - Professionals tend to use a faraday cage/mesh to implement electromagnetic shielding, but it can be pricey for this scenario.
   - A quick do-it-yourself solution (as proof-of-concept) would be to wrap the PA/LNA module with electrical tape and then wrap foil around the electrical tape (for shielding) while being very careful to not let the foil touch any current carrying parts (like the GPIO pins, the antenna mount, and the soldier joints for the antenna mount). Observe
   [![ghetto_shielding_1.png](https://github.com/nRF24/RF24/blob/master/images/ghetto_sheilding_1.png)](https://github.com/nRF24/RF24/blob/master/images/ghetto_sheilding_1.png)
   [![ghetto_shielding_2.png](https://github.com/nRF24/RF24/blob/master/images/ghetto_sheilding_2.png)](https://github.com/nRF24/RF24/blob/master/images/ghetto_sheilding_2.png)

### My PA/LNA module doesn't perform as well as I'd hoped or the NRF radio works better on touching it

As described above, the radio modules (notably the PA+LNA versions) are reliant on a stable power source. While these modules may work with a poor power supply, they often lose packets or fail to receive as many as a module with a better power source. Moreover, this can sometimes be seen in odd ways such as the radio module working better when touched. This again is likely a power stability issue because the radio module is missing a capacitor (a commonly neglected expense on behalf of the module's manufacturer).

Add capacitor(s) close to the VCC and GND pins of the radio. Typically, 10uF is enough. Depending upon your circuit's layout, differences in capacitors' electrolytic properties can be seen, such that a low ESR (Equivalent Series Resistance) rated capacitor is desirable.
