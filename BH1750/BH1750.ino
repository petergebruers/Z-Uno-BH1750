
/*
  Attemp at an improved version of BH1750 sketch published by Z-Wave.me

  Author: Peter Gebruers, aka "petergebruers". Meet me at:

  https://forum.fibaro.com/index.php?/profile/1799-petergebruers/
  or:
  https://forum.z-wave.me/memberlist.php?mode=viewprofile&u=564463

  The full project is on Github:

  https://github.com/petergebruers/Z-Uno-BH1750

  Licensing, warranty and liability, governed by:
  "Attribution 4.0 International (CC BY 4.0)", available at:

  https://creativecommons.org/licenses/by/4.0/

  Additional information on Github in files README and LICENSE

  Version 1.0 public release, based on the code published at:

  http://z-uno.z-wave.me/examples/bh1750/
*/

// INCLUDES

#include "Wire.h"

// ****** USER CONFIGURABLE DATA BEGIN ****** //
// Please check if these values match your setup!

// BH1750 can have 2 possible addresses on the bus, 0x23 ("Low") or 0x5C ("High")
// If white LED blinks 1 second, I2C communication is NOK and you might want to check
// the adddress...
#define BH1750_I2CADDR 0x23

// LED_BUILTIN is pin number 13 = white user LED of Z-Uno board

#define LED_PIN LED_BUILTIN

// ****** USER CONFIGURABLE DATA END ****** //

// DEBUGGING HELPER MACROS
//
// Can be used to remove all debugging statements. See below.
//
// Default: print debugging info on USB serial port (same as programming port).

#ifdef PG_DEBUG_PRN
#error DOH! PG_DEBUG_PRN defined in another file!
#endif

#ifdef PG_DEBUG_INIT
#error DOH! PG_DEBUG_INIT defined in another file!
#endif

// Comment the next two lines and uncomment the next pair if
// you want to remove debugging.
// This has might have a perfomance impact.
#define PG_DEBUG_PRN(...) Serial.print(__VA_ARGS__)
#define PG_DEBUG_INIT() Serial.begin()

// #define PG_DEBUG_PRN(...)
// #define PG_DEBUG_INIT(...)

// CLASSES //

// It think it is important to define classes before globals.
// Failure to do so might give uCxx compiler errors.

// Simplified BH1750 interface, based on Z-Uno 2.1.0 BH1750 library.
// I cannot use the library included on the Z-Uno platform, because:
// a) It does not support ligt levels above about 13000 Lux
// b) It does not let you know if I2C was OK or not.

class BH1750 {
#define BH1750_CONTINUOUS_HIGH_RES_MODE 0x10
  private:
    byte sensorAddress;

  public:
    BH1750();

    // lightLux is luminance measured in Lux.
    // Type long is 32-bit signed, which may seem odd for luminance, but
    // Z-wave uses signed data for reporting, so that is what I use!
    long lightLux;

    /* byte I2Cstatus meaning:
    *
    * 0:success
    * 1:data too long to fit in transmit buffer
    * 2:received NACK on transmit of address
    * 3:received NACK on transmit of data
    * 4:other error
    */
    byte I2Cstatus;

    void begin(uint8_t mode = BH1750_CONTINUOUS_HIGH_RES_MODE, uint8_t addr = 0x23);

    void readLightLevel();
};

BH1750::BH1750() {};

void BH1750::begin(uint8_t mode, uint8_t addr) {
  sensorAddress = addr;
  Wire.begin();
  Wire.beginTransmission(sensorAddress);
  Wire.write(mode);
  I2Cstatus = Wire.endTransmission();
}

void BH1750::readLightLevel() {
  Wire.requestFrom(sensorAddress, 2);
  I2Cstatus = Wire.getStatus();
  if (I2Cstatus != 0) return;

  ((byte *)&lightLux)[3] = 0;
  ((byte *)&lightLux)[2] = 0;
  ((byte *)&lightLux)[1] = Wire.read();
  ((byte *)&lightLux)[0] = Wire.read();

  // Actual Lux = sensor out * 1.2.
  // See datasheet of BH1750.

  lightLux *= 6;
  lightLux /= 5;
}


// Global variables

// Z-Uno is a SOC with limited resources compared to e.g a Raspberry Pi
// or esp-32. To limit stack usage and parameter passing and data shuffling,
// I tend to use global variables instead of local or parameter passing. This goes against
// the idea of "abstraction" and "isolation of data". Whether this is better
// for such a small project is debatable...

// BH1750 is a class that interfaces with the sensor.
BH1750 Sensor;

// Clock10s counts "multiples of 10 seconds" to limit unsolicited reporting.
// By setting it to 254 I pretend the last update was long ago,
// So after poweron the sensor immediately reports light level.
byte Clock10s = 254;

// msCounter1, msCounter2 are used to cound milliseconds
// in the timer interrupt, to derive 1s and 10s pulses.
short msCounter1, msCounter2;

// LedBlip is used to cause the while LED to emmit a very short pulse of light.
// It is used in the getter to signal the Z-Uno sends data.
boolean LedBlip;

// Delay10s is used to separate the unsolicited reports.
// If Delay10s is 3 then an unsolicited report will be send
// after 30 seconds.
byte Delay10s;

// Pulse1s acts as a 1 second clock for timing the main loop.
// Pulse1s is set when msCounter1 counts down from 1000 to 0.
// It is used instead of delay because it is non-blocking.
boolean Pulse1s;

// PrevLightLux stores the previous light measurement. It is used
// to calculate the time between unsolicited reports.
long PrevLightLux;

// Global functions

// Timer is called by the 1ms interrupt routine of the Z-Uno
// to measure time between reports and pulse the loop once every second.

void Timer() {
  if (msCounter1 == 0)
  {
    msCounter1 = 1000;
    Pulse1s = true;
  }
  else
    msCounter1--;

  if (Clock10s == 0)
  {
    msCounter2 = 0;
  }
  else if (Clock10s == 255) {
    return;
  }

  if (msCounter2 == 0)
  {
    // You can speed up the reporting for debugging, by setting msCounter2
    // to a lower valua, e.g. 1000 = 10 x faster.
    msCounter2 = 10000;
    Clock10s++;
  }
  else
    msCounter2--;
}

// ZUNO_SETUP_ISR_1MSTIMER is a predefined macro that attaches
// a user defined function to the 1ms timer of the Z-Uno core.
ZUNO_SETUP_ISR_1MSTIMER(Timer);

// CalculateDelay sets global variable Delay10s based on globals
// PrevLightLux and Sensor.lightLux. It has no return value.
// It is a separate function, because it makes testing easier.
// It allows me to copy/paste this code in Code::Blocks and run it on my PC
// with some simulated data.
// CalculateDelay sets a short delay if "light changes a lot" and
// a "long delay" id "light does not change a lot".
// It also considers light values <=2 to mean "dark" to avoid sending
// to many reports due to noise.
// I am not proud of this code: it is functional, but not elegant.

// DELAY_STEPS is the number amount of "doubling" of the standard time interval.
// A standard time interval is 30 seconds. Setting DELAY_STEPS to 6 means
// if the data does not change a lot, report intervall will be set
// to 30 * 2^6 = 1920 seconds = 32 minutes.
#define DELAY_STEPS 6

void CalculateDelay()
{
  long delta, reference;

  PG_DEBUG_PRN("\nPrevLightLux: ");
  PG_DEBUG_PRN(PrevLightLux);
  PG_DEBUG_PRN(", lightLux: ");
  PG_DEBUG_PRN(Sensor.lightLux);

  delta = Sensor.lightLux - PrevLightLux;

  if (delta > 0)
  {
    reference = PrevLightLux;
  }
  else
  {
    reference = Sensor.lightLux ;
    delta = -delta;
  }

  if (delta < 2)
  {
    delta = 0;
  }

  PG_DEBUG_PRN(", delta: ");
  PG_DEBUG_PRN(delta);

  Delay10s = 3;
  if (delta == 0)
  {
    Delay10s = (3 << DELAY_STEPS);
  }
  else
  {
    while (delta < reference)
    {
      Delay10s += Delay10s;
      if (Delay10s >= (3 << DELAY_STEPS))
      {
        break;
      }
      delta += delta;
    }
  }

  PG_DEBUG_PRN(", Clock10s: ");
  PG_DEBUG_PRN(Clock10s);
  PG_DEBUG_PRN(", Delay10s: ");
  PG_DEBUG_PRN(Delay10s);
}

// set up channels
// We need SENSOR_MULTILEVEL_SIZE_FOUR_BYTES because the BH1750
// can report up to 78642 Lux and that does not fit into two bytes.
ZUNO_SETUP_CHANNELS(
  ZUNO_SENSOR_MULTILEVEL(ZUNO_SENSOR_MULTILEVEL_TYPE_LUMINANCE,
                         SENSOR_MULTILEVEL_SCALE_LUX,
                         SENSOR_MULTILEVEL_SIZE_FOUR_BYTES,
                         SENSOR_MULTILEVEL_PRECISION_ZERO_DECIMALS,
                         GetterLightLux)
);

void setup() {

  // Note: you cannot print to serail in setup(). That is why I
  // use a blinking LED to signal I2C init error.
  // setup should be short/fast. You cannot program the Z-Uno
  // while setup() is running.

  PG_DEBUG_INIT();

  // Start continous measurement in HIGH_RES_MODE at 1 Lux resolution
  Sensor.begin(BH1750_CONTINUOUS_HIGH_RES_MODE, BH1750_I2CADDR);
}

void loop() {
  if (LedBlip) {
    digitalWrite(LED_PIN, 1);
    delay(2);
    digitalWrite(LED_PIN, 0);
    LedBlip = false;
    PG_DEBUG_PRN("\nData Reported.\n");
  }

  if (Pulse1s == false) {
    return;
  }

  Pulse1s = false;

  // Code below runs 1x per second.

  if (Sensor.I2Cstatus != 0)  {
    int i;

    // Turn on/off the white LED a number of times at 10 Hz, this
    // causes a visible flicker effect, to signal the user an error has
    // occured.
    for (i = 0; i < 5; i++) {
      digitalWrite(LED_PIN, 1);
      delay(10);
      digitalWrite(LED_PIN, 0);
      delay(90);
    }
    Sensor.begin(BH1750_CONTINUOUS_HIGH_RES_MODE, BH1750_I2CADDR);
    PG_DEBUG_PRN("\nI2C error occured. I2C reset.\n");
    return;
  }

  Sensor.readLightLevel();

  if (Sensor.I2Cstatus != 0) {
    return;
  }

  CalculateDelay();

  if (Clock10s > Delay10s) {
    zunoSendReport(1);
  }
}

// petergebruers: Value returned by getter is signed. The Z-Uno reference says to use DWORD.
// I cannot make it work with DWORD, but type "long" is OK.
long GetterLightLux() {
  Clock10s = 0;
  LedBlip = true;
  PrevLightLux = Sensor.lightLux;
  return Sensor.lightLux;
}
