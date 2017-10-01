# Motivation

After I've toyed with the Z-Uno demo sketches (see http://z-uno.z-wave.me) for some time, I thought I would show a little ambition and at the same time seek feedback from experienced users. I took the BH1750 sketch and improved the reporting range (from about 13000 to about 79000 Lux), add detection of I2C bus errors and add an algorithm to calculate  reporting interval, getting good responsiveness without spamming the Z-Wave network.  I would really appreate if you could review it, or even try it. Then give me some comment, criticism or advice! I will use it to refine this sketch and develop other solutions!

# Features

- Measures 1 x per second, so you can poll at any time and get a value that is no older than about 1 second.
- Reports 0 - 78642 Lux. Resolution: 1.2 Lux (Original Demo Sketch: max about 13000 Lux).
- Plenty of debugging output via USB (serial emulation). Can be disabled by changing  two lines in the sketch.
- Honours minimum 30 second reporting interval for unsollicited reports.
- Automatic reporting interval, ranging from 30 seconds up to 32 minutes. If light level doubles or halves, the sensor reports as soon as possible (honouring the minimum of 30 seconds between unsollicited reports). If changes are smaller, reporting time increases. Tested by me, indoors and outdoors. (Original Demo Sketch: fixed reporting).
- Detects I2C bus errors. LED blinks+ automatic bus  reset. Sensor can be hot plugged. (Original Demo Sketch: no detection of bus errors, sketch does not recover from bus errors).
- Plenty of comments in source code.
- Uses timer(s) and avoids delay() to get maximum responsiveness.
- White LED blips when data gets transferred to the controller.

# Requirements

- Tested on Z-Uno firmware 2.1.0, but probably works on older and newer version too.

# Installation and download

- Make sure you have installed Arduino and the Z-Uno board definition.
  - http://z-uno.z-wave.me/install
- Make sure you have the Windows driver
  - http://z-uno.z-wave.me/install/driver
- Download this sketch from Github:
  -  https://github.com/petergebruers/Z-Uno-BH1750
- Check Arduino, Tools, Frequency. The default is Russia. If you are unable to include your Z-Uno, double check this setting and re-upload if it was wrong.
- Exclude your Z-Uno from your controller, because you probably are not having the same channel definitions as me, if you have installed another sketch befor this one.
- Check the address of your BH1750 at the start of the sketch.
- Upload the sketch to your Z-Uno.
- Include the Z-Uno. After the interview (Z-Way) or addition of the modules finishes (Fibaro Home Center), you should see 2 light sensors. They are duplicates, because that is how Z-Wave makes this device compatible with older controllers. It is normal.

# Troubleshooting

- - If the white LED is blinking nervously, I2C bus errors occurred. Maybe your BH1750 is on the alternate address. Or maybe your connections are faulty.
- You should see a short flash of light each time the sensor reports. This happens when you "poll" the device from the controller or the algortihm decides it is time to send an update to the controller. After such a report, the sensor does not send updates for 30 seconds, unless you "poll it". This is by design.
- If you set your ardiuno serial monitor to 115200 bps you can check what is happening on your Z-Uno.
- You can change the reporting timer in the code to run 10 or 100 times faster. This makes debugging easier: instead of having to wait a few minutes (for a small change in light level) you can see debugging in seconds...
- If you need help, check "About the author" below.

# Possible future improvements or variants

- - Set BH1750 to 0.5 Lux mode to achieve 0.6 Lux resolution and max 39321 Lux
- Set reporting scale to 1 instead of 0 decimal, so a decimal fraction can be reported for really low light levels.
- After release of Z-Uno framework 2.1.1: add parameters for configuration. For example, disable "LED blip" if users consider this to be annoying.

# Revision list

- 1.0.0: Initial public release.

# Licensing, warranty and liability

 This sketch and all parts are governed by: Creative Commons "Attribution 4.0 International (CC BY 4.0)", available at:  https://creativecommons.org/licenses/by/4.0/

# About the author

- I'm Peter Gebruers, aka "petergebruers", and I live in Flanders near Brussels. I am an engineer and I've worked mostly in IT as a systems/network engineer.
- I am quite new to Z-Uno development but I do have some experience with microcontrollers and SoC like PIC 8 bit, STM32, ESP8266. It is a hobby. I own two Z-Uno Boards.
- I started my Home Automation System in 2012, based on a Fibaro HC2. Later I've added a RaZberry Pi. I have about 100 physical nodes.
- Contact me on one of these forums:
  - https://forum.fibaro.com/index.php?/profile/1799-petergebruers/
  - https://forum.z-wave.me/memberlist.php?mode=viewprofile&u=564463
