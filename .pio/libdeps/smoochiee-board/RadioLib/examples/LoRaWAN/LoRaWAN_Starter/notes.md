# RadioLib LoRaWAN on TTN starter script

## Welcome

These notes are for someone who has successfully created a few sketches for their Arduino based device but is starting out with LoRaWAN. You don't have to be a C++ coding ninja but some familarity with C++ and procedural programming is assumed. The absolutely simplest way to get started is to buy some known good hardware that's all done for you so you can concentrate on the code & configuration.


## Introduction

LoRaWAN is an amazing system for small battery powered sensors collecting data for years at a time. With great features comes some more complex elements which means it is not quite as simple as just providing WiFi credentials and pushing data through. It is in the range of setting up & customising the settings for a home router but with no wizards to do the heavy lifting for you. So we strongly recommend spending a couple of hours reviewing the TTN Getting Started section so you are aware of the minimum knowledge to make a successful start: https://www.thethingsnetwork.org/docs/lorawan/. Johan's video is amazing but is also drinking from the firehose. Read the text first and then watch the video on Youtube where there are bookmarks to deliver it in small digestable chunks.

These notes plus a lot more are available in the wiki: https://github.com/jgromes/RadioLib/wiki/LoRaWAN

For questions about using RadioLib there is the discussions section (https://github.com/jgromes/RadioLib/discussions) and if you believe you've found an issue (aka bug), the issues section (https://github.com/jgromes/RadioLib/issues). If posting an issue please ensure you tell us what hardware you are using and provide a debug log - make sure you enable `RADIOLIB_DEBUG_PROTOCOL`. If the question is more LoRaWAN or firmware related, then you can use the TTN forum: https://www.thethingsnetwork.org/forum/


## Register & setup on TTN

This sketch isn't particularly aimed at The Things Stack (TTS) but you can get a free Sandbox account and the following instructions are for that. Chirpstack works just as well, but the buttons and labels may have different names. We discourage the use of Helium as it does not support LoRaWAN v1.1 and has some other odd limitations. Other LoRaWAN Network Servers (LNS) have not been tested by the developers, so YMMV.

Why no screen shots? TTS is a web based app, one that you will need to become familiar with and we will need to direct you to some of the less obvious parts. So much better that you learn the layouts in concept than slavishly follow screen shots that can & will go stale.

There will be some instructions that you have to take on face value. You didn't learn to run before you walked and it's so much more encouraging to get started and build on success than get bogged down in endless details. Once you are up & running more of the details start to slot in to place.

### Register on TTN

Go to https://www.thethingsnetwork.org/get-started and register - just like any other website. These instructions are for TTS Sandbox.

Once you have confirmed your email address, you can login to the console here: https://console.cloud.thethings.network/. If you allow your browser to share your location the best console will be selected. For most users the best one is the obvious one, if you have any doubts you can ask on the forum here: https://www.thethingsnetwork.org/forum/ - you login with the exact same details.

It is simpler to register your gateway first. If you don't have a gateway, then a The Things Indoor Gateway (TTIG) is a very affordable option. A gateway gives you a console to see if your device is being heard and is hugely useful when debugging a DIY device. If you are in range of a community gateway you may be lucky with your first device creation but you will never know if you are in range unless you have access to that gateway's console.

You can read up on key concepts and troubleshooting here: https://www.thethingsindustries.com/docs/gateways/

LoRa stands for Long Range - having the gateway & device on the same desk tends to overload both receiver circuits when they hear a transmission so close to hand. The gateway should be 5 - 10m away, preferably with a solid wall in the way as well.

### Create your application

An application is like a box to keep some devices in - normally doing the same thing - on larger deployments this may be 1,000's of similar devices. Starting out it is likely to be just a few so there is no need to get concerned about how to divide up your use just yet.

Onced logged in to the console you can go in to Applications to create your first application. The ID must be all lower case or numbers, no spaces, dashes are OK and it has to be unique to the entire TTN community - so `first-app` will be rejected - you could use `your-username-first-app` as that's likely to be unique. The name and description are for your own use and are optional.

The main menu for an application is in the left hand panel - nothing is needed there just yet.

### Create your device

On the right hand side about half way down on your application's summary is a big blue button `+ Register end device`. Click this to create the settings for your first device.

You are making your own device using a third party LoRaWAN stack so there will not be an entry in the device repository so choose 'Enter end device specifics manually'.

Choose the Frequency plan appropriate for your region. Consider that almost all countries have laws relating to what frequencies you use so don't get creative. For Europe please use the recommended option. For other regions use the entry marked 'used by TTN'.

Choose LoRaWAN 1.1.0 - the last one in the list - the latest specfication. RadioLib uses RP002 Regional Parameters 1.0.4.

At this point you will be asked for your JoinEUI. As this is a DIY device and we are using RadioLib, you can use all zero's as recommended by The LoRa Alliance TR007 Technical Recommendations document. Once you've put in all zeros and clicked confirm you will be asked for a DevEUI, AppKey and NwkKey. It is preferable to have the console generate them so they are properly formatted.

Your End device ID can be changed to make the device more identifiable. Something related to your hardware helps - like `devicename-01`. The you can click the blue 'Register device'.

When retail sensors are being deployed, a device is registered, batteries put in, it joins and gets on with sending data for the next few years. For development purposes however we need to turn off one of the security settings so that you can join & uplink out of the normal sequence that a device in the field would do.

Click on General Settings, scroll down to Join settings, click the Expand button, scroll down and click the 'Resets join nonces' option. You will see a warning about replay attacks which is entirely proper & correct. If anyone eavesdropping in your area on your LoRa transmissions could fake a join and send uplinks from their device but only if they happened to find out your AppKey & NwkKey which is kept securely on the TTN servers and is never transmitted over the air, so they'd also have to login to your account, which is protected by your password. 

You then need to copy over the device details in to the config file for RadioLib. There are buttons to copy items to the clipboard so you don't have to hand type them.

### Copy & Paste made easy

You can copy the EUIs & keys from the device overview section.

The EUIs are really straightforward - click the clipboard icon at the right hand end of the EUI display field and it will be copied in the format you need. You can then paste it in to the code - you must leave the 0x in place so the compiler knows that it's a hex value.

The keys are relatively straightforward. Click the eye icon at the right hand end of the field. Then click the <> icon that will appear to the left. This will format the hex values as an array. Then you can click the clipboard icon to copy the array and then paste it between the { } brackets.

### Secrets to keep safe.

The Join & Dev EUI's are transmitted in plain text when the device joins a network. The gateway ID is public. If you have an issue and are asked for details, there are only three things to keep private - your password, the keys which are used for encryption and any API keys you create which are used for accessing your data & configuration.


### Monitoring your device

If you are on your application summary page you'll see uplinks in the small activity box top right with a link to the full size table. If you click the Live Data menu item on the left it will show activity for all the devices registered on the application in the full window.

If you just want your devices activity, from the summary page click on the device in the list in the middle of the page.

The main menu for a device is the horizontal band: Overview, Live Data, Messaging etc. You can click Live Data or the link above the small activity box.

**The console shows LIVE data - not a history of everything that has ever happened. A LNS is a management & relay service, not a database. When you open the console you may see a summary of recent activity - this is a bonus. You must leave the console open, even in another tab, if you want to see live activity.**


### Explore

Nothing on the console can be upset unless you confirm a warning message, so you are safe to explore the different menus to orientate yourself. This is very good idea so you have an understanding of the layout of the land and shouldn't take more than 10 or 15 minutes. The documentation & volunteers on GitHub and the TTN forum will make refer to parts of the console without giving blow by blow directions.




## The config.h

### The uplinkInterval

LoRaWAN devices typically send small amounts of data at intervals between 15 minutes through to once per day. This allows a device to run on two AA batteries for 2 to 5 years. Hoping that LoRaWAN can move lots of data and your device can regularly receive commands to do something on demand is trying to bend the LoRaWAN system in ways it is not designed for and usually ends up with far too many issues to unravel.

The radio frequencies that are used are usually shared with other Industrial, Scientific & Medical, known as ISM, users. The LoRa modulation is particularly resistant to interference due to other simultaneous transmissions on the same frequency but too much local activity will mean that not all uplinks get through. The Things Industries suggest designing a system to a potential packet loss rate of 10%. Typically we see 1 or 2% loss. This is entirely down to shared use of the radio waves, once an uplink is heard by a gateway the system is super reliable through The Things Stack.

To ensure that the shared ISM bands are fairly used there are limits defined in law on how often you can transmit, called Duty Cycle. The details vary by region or country but typically you can only transmit for 1% of the time. Some frequencies you can only use 0.1% of the time. See https://www.thethingsnetwork.org/docs/lorawan/duty-cycle/ for more information.

Additionally, as The Things Stack Sandbox aka TTN is an array of servers in three locations around the world paid for by The Things Industries, there is a Fair Use Policy so that those learning LoRaWAN, communities, hobbyists & makers are guided on how much of the resource any one device can use. In short, it's 30 seconds of airtime a day and 10 downlinks. When a gateway is transmitting a downlink it can not hear any uplinks (contributing to the potential uplink loss outlined above). The community consensus is that 1 downlink a fortnight to update or adjust settings is appropriate. See https://www.thethingsnetwork.org/docs/lorawan/duty-cycle/#fair-use-policy for more information.

You can see what intervals can be used with this interactive calculator: https://avbentem.github.io/airtime-calculator/ttn/. Devices further away from gateways will have to use a higher Spread Factor to be heard - do not assume everything will happen at SF7. An uplink takes a minimum of 6 seconds from start to end, sometimes longer if the device is further away from the gateway, so you will need to be patient for just a short while whilst waiting for feedback after seeing "Sending uplink"

With all these considerations, trying to use LoRaWAN for command & control isn't appropriate and realtime GPS tracking almost always breaches FUP and usually legal limits, leaving aside the challenges of coverage.

See the hints & tips section on testing your device.


### EUI's & Keys

In the `config.h` towards the top there are four lines thus:

// replace-with-your-device-id
uint64_t joinEUI =   0x0000000000000000;
uint64_t devEUI  =   0x0000000000000000;
uint8_t appKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t nwkKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

On the TTN console on the device summary page, click the clipboard icon next to the DevEUI, highlight the 16 0's in the third line after the x and paste.

The devEUI must start with 0x and will end up looking something like 0x70B3D57ED006544E

For the appKey we need TTN to format it correctly. Click the eye icon and an extra icon will appear <> - click this and the key will be formatted for you. Click the clipboard icon and then paste over the 32 0x00's in the config file. Then do the same for nwkKey.

A key will end up something like 0x31, 0x16, 0x6A, 0x22, 0x97, 0x52, 0xB6, 0x34, 0x57, 0x45, 0x1B, 0xC3, 0xC9, 0xD8, 0x83, 0xE8


### Region

The region value you use MUST match the one you selected on the console. 

If you are using US915 or AU915 then you should change the subBand const to 2.

### The pinmap

This is the connection between your microcontroller (ESP32, ATmega, SAMD etc.) and the radio (SX1276, SX1262, LR1110 etc.).
You have to select the correct module and set the correct pins.

Pin maps for commonly used radio modules are kept in a separate library, called RadioBoards: https://github.com/radiolib-org/RadioBoards

It can automatically detect your microcontroller platform and radio, and configure things for you. If you have a board that is not supported by RadioBoards, feel free to suggest it in the RadioBoards issues, or better yet - open a pull request there!


## Observations on the main sketch

Most of the sketch has comments that tell you what the various parts are doing. This should add a little more info:

### The Join

When a device is first started, it needs to register with the LoRaWAN Network Server (LNS) and setup it's session. With the settings from the console copied over and a gateway an appropriate distance away, most of the time the join will 'just work'.

If it doesn't, then there is no point trying repeatedly without going through the troubleshootng sequence. So this starter sketch will try once only to save the airwaves & TTN Community servers from repeated misfires.


### The payload

You may see other starter sketches sending text. Apart from being massively inefficient, the text isn't easily displayed on the TTN console which makes it rather pointless and pro embedded engineers don't send strings. So this sketch sends the data as a sequence of bytes as recommended.

Further reading on this can be found here, just ignore the pink message about v2, it's all still valid: https://www.thethingsnetwork.org/docs/devices/bytes/

We've not assumed anything about any sensors you have, so we are just reading a digital & an analog pin. An analog reading is typically a two byte value - an integer - this is split using the Arduino highByte & lowByte function. You'll see how we put it back together in the TTN console below.


## TTN Console Payload Decoder

Coming soon

## Hints & Tips

### Device testing

The LoRaWAN code base works to a specification and once you are happy your device is able to join & send a few dozen uplinks, continuing to sit around waiting for an uplink to test your sensor code & payload format is a waste of your time. The solution is to write everything else in a different sketch, output the array to the serial console and then you can copy & paste the hex array in to the TTN console Payload Formatters section to test the decoding.
