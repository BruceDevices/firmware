"""A simple script to take all data dumped from the nRF24L01 registers and
output it in human readable form.

Example usage:
    print_details.py "0e 3f 02 03 00 02 00 0e"

Notes:
    * The radio's power state is represented under the assumption that
      the radio's CE pin is inactive low.
"""
# pylint: disable=consider-using-f-string
import struct
import argparse


def hex_str_to_bytes(s_in: str) -> bytes:
    """Used to convert a string from CLI to a bytearray object.

    .. warning:: This function assumes that the string consists of only
        hexadecimal characters.
    """
    return bytes([int(i, 16) for i in s_in.split()])


argparser = argparse.ArgumentParser(
    description=__doc__,
    formatter_class=argparse.RawDescriptionHelpFormatter,
)
argparser.add_argument(
    "buffer",
    type=hex_str_to_bytes,
    help="The encoded buffer from the Arduino Serial Monitor. The string passed is "
    "expected to contain only hexadecimal digits. It should be 38 words long "
    "(separated by spaces).",
)


def address_repr(buf, reverse: bool = True, delimit: str = "") -> str:
    """Convert a buffer into a hexadecimal string."""
    order = range(len(buf) - 1, -1, -1) if reverse else range(len(buf))
    return delimit.join(["%02X" % buf[byte] for byte in order])


# pylint: disable=too-many-locals,too-many-statements
def print_details(encoded_buf: bytearray):
    """This debugging function outputs all details about the nRF24L01."""
    # declare sequences
    pipes = [bytearray(5)] * 2 + [0] * 4
    pl_len = [0] * 6

    # unpack bytearray
    (
        config,  # 0x00
        auto_ack,  # 0x01
        open_pipes,  # 0x02
        addr_len,  # 0x03
        retry_setup,  # 0x04
        channel,  # 0x05
        rf_setup,  # 0x06
        status,  # 0x07
        observer,  # 0x08
        rpd,  # 0x09
    ) = struct.unpack("10B", encoded_buf[:10])
    pipes[0] = encoded_buf[10:15]  # 0x0A
    pipes[1] = encoded_buf[15:20]  # 0x0B
    (
        pipes[2],  # 0x0C
        pipes[3],  # 0x0D
        pipes[4],  # 0x0E
        pipes[5],  # 0x0F
    ) = struct.unpack("4B", encoded_buf[20:24])
    tx_address = encoded_buf[24:29]  # 0x10
    (
        pl_len[0],  # 0x11
        pl_len[1],  # 0x12
        pl_len[2],  # 0x13
        pl_len[3],  # 0x14
        pl_len[4],  # 0x15
        pl_len[5],  # 0x16
        fifo,  # 0x17
        dyn_pl,  # 0x1C
        features,  # 0x1D
    ) = struct.unpack("9B", encoded_buf[29:38])
    ce_pin, csn_pin, spi_speed = struct.unpack(">2H1B", encoded_buf[38:44])

    # do some deciphering arithmetic
    addr_len += 2
    crc = (2 if config & 4 else 1) if auto_ack else max(0, ((config & 0x0C) >> 2) - 1)
    d_rate = rf_setup & 0x28
    d_rate = (2 if d_rate == 8 else 250) if d_rate else 1
    pa_level = (3 - ((rf_setup & 6) >> 1)) * -6
    pa_level = (
        "MIN"
        if pa_level == -18
        else ("LOW" if pa_level == -12 else ("HIGH" if pa_level == -6 else "MAX"))
    )
    dyn_p = (
        ("_Enabled" if dyn_pl else "Disabled")
        if dyn_pl == 0x3F or not dyn_pl
        else "0b" + "0" * (8 - len(bin(dyn_pl))) + bin(dyn_pl)[2:]
    )
    auto_ack = (
        ("Enabled" if auto_ack else "Disabled")
        if auto_ack == 0x3F or not auto_ack
        else "0b" + "0" * (8 - len(bin(auto_ack))) + bin(auto_ack)[2:]
    )
    pwr = "Standby" if config & 2 else "Off"
    is_plus_variant = bool(spi_speed >> 4)
    spi_speed = spi_speed & 0xF

    # print it all out
    print("CE pin____________________{}".format(ce_pin))
    print("CSN pin___________________{}".format(csn_pin))
    print("SPI speed_________________{} MHz".format(spi_speed))
    print("Is a plus variant_________{}".format(is_plus_variant))
    print(
        "Channel___________________{}".format(channel),
        "~ {} GHz".format((channel + 2400) / 1000),
    )
    print(
        "RF Data Rate______________{}".format(d_rate),
        "Mbps" if d_rate != 250 else "Kbps",
    )
    print("RF Power Amplifier________PA_{}".format(pa_level))
    print(
        "RF Low Noise Amplifier____{}abled".format(
            "En" if bool(rf_setup & 1) else "Dis"
        )
    )
    print("CRC Length________________{} bits".format(crc * 8))
    print("Address length____________{} bytes".format(addr_len))
    print("TX Payload lengths________{} bytes".format(pl_len[0]))
    print(
        "Auto retry delay__________{} microseconds".format(
            ((retry_setup & 0xF0) >> 4) * 250 + 250
        )
    )
    print("Auto retry attempts_______{} maximum".format(retry_setup & 0x0F))
    print("Re-use TX FIFO____________{}".format(bool(fifo & 64)))
    print("Received Power Detected___{}".format(bool(rpd)))
    print(
        "Packets lost on current channel_____________________{}".format(observer >> 4)
    )
    print(
        "Retry attempts made for last transmission___________{}".format(observer & 0xF)
    )
    print(
        "IRQ on Data Ready__{}abled".format("Dis" if config & 64 else "_En"),
        "   Data Ready___________{}".format(bool(status & 0x40)),
    )
    print(
        "IRQ on Data Sent___{}abled".format("Dis" if config & 32 else "_En"),
        "   Data Sent____________{}".format(bool(status & 0x20)),
    )
    print(
        "IRQ on Data Fail___{}abled".format("Dis" if config & 16 else "_En"),
        "   Data Failed__________{}".format(bool(status & 0x10)),
    )
    print(
        "TX FIFO full__________{}e".format("_Tru" if fifo & 0x20 else "Fals"),
        "   TX FIFO empty________{}".format(bool(fifo & 0x10)),
    )
    print(
        "RX FIFO full__________{}e".format("_Tru" if fifo & 2 else "Fals"),
        "   RX FIFO empty________{}".format(bool(fifo & 1)),
    )
    print(
        "Multicast__________{}ed    Custom ACK Payload___{}abled".format(
            "_Allow" if features & 1 else "Disabl",
            "En" if features & 2 else "Dis",
        ),
    )
    print("Dynamic Payloads___{}    Auto Acknowledgment__{}".format(dyn_p, auto_ack))
    print(
        "Primary Mode_____________{}X".format("R" if config & 1 else "T"),
        "   Power Mode___________{}".format(pwr),
    )
    print("TX address____________ 0x{}".format(address_repr(tx_address)))
    for i in range(6):
        is_open = open_pipes & (1 << i)
        address = pipes[i] if i < 2 else bytes([pipes[i]]) + pipes[1][1:]
        print(
            "Pipe {} ({}) bound: 0x{}".format(
                i, " open " if is_open else "closed", address_repr(address)
            ),
        )
        if is_open and not dyn_pl & (1 << i):
            print("\t\texpecting {} byte static payloads".format(pl_len[i]))


# pylint: enable=too-many-locals,too-many-statements


if __name__ == "__main__":
    args = argparser.parse_args()
    print_details(args.buffer)
