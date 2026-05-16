# LoRaWAN examples
RadioLib LoRaWAN examples.

* [LoRaWAN_Starter](https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN/LoRaWAN_Starter): this is the recommended entry point for new users. Please read the [`notes`](https://github.com/jgromes/RadioLib/blob/master/examples/LoRaWAN/LoRaWAN_Starter/notes.md) that come with this example to learn more about LoRaWAN and how to use it in RadioLib!
* [LoRaWAN_Reference](https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN/LoRaWAN_Reference): this sketch showcases most of the available API for LoRaWAN in RadioLib. Be frightened by the possibilities! It is recommended you have read all the [`notes`](https://github.com/jgromes/RadioLib/blob/master/examples/LoRaWAN/LoRaWAN_Starter/notes.md) for the Starter sketch first, as well as the [Learn section on The Things Network](https://www.thethingsnetwork.org/docs/lorawan/)!
* [LoRaWAN_ABP](https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN/LoRaWAN_ABP): if you wish to use ABP instead of OTAA, this example shows how you can do this using RadioLib. However, to comply with the specification, the full session must persist through resets and power loss - you would need proper NVM for this. Really, we recommend using OTAA.
* [LoRaWAN_Class_C](https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN/LoRaWAN_Class_C): this shows how to use Class C on top of Class A. This is useful for continuously-powered devices (no batteries) such as lights. If you deploy multiple similar devices, please use Multicast instead.
* [LoRaWAN_Multicast](https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN/LoRaWAN_Multicast): a showcase of Multicast over Class C. This is particularly useful for groups of devices such as a series of street lights.

> [!CAUTION]
> These examples are quick wins during development. However, for production devices, you will need to add persistence to your device. See the wiki for more details, or head straight to some [persistence examples](https://github.com/radiolib-org/radiolib-persistence).

> [!TIP]
> Refer to the [Wiki](https://github.com/jgromes/RadioLib/wiki/LoRaWAN) for guides and information, for example on LoRaWAN versions, registering your device and more.
