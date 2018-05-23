/*
  AnalogFirmata.h - Firmata library
  Copyright (C) 2006-2008 Hans-Christoph Steiner.  All rights reserved.
  Copyright (C) 2010-2011 Paul Stoffregen.  All rights reserved.
  Copyright (C) 2009 Shigeru Kobayashi.  All rights reserved.
  Copyright (C) 2013 Norbert Truchsess. All rights reserved.
  Copyright (C) 2009-2015 Jeff Hoefs.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.

  Last updated by Jeff Hoefs: November 22nd, 2015
*/

#include <ConfigurableFirmata.h>
#include "AnalogFirmata.h"
#include "AnalogInputFirmata.h"

AnalogInputFirmata *AnalogInputFirmataInstance;

void reportAnalogInputCallback(byte analogPin, int value)
{
  AnalogInputFirmataInstance->reportAnalog(analogPin, value);
}

AnalogInputFirmata::AnalogInputFirmata()
{
  AnalogInputFirmataInstance = this;
  reset();
  Firmata.attach(REPORT_ANALOG, reportAnalogInputCallback);
}

// -----------------------------------------------------------------------------
/* sets bits in a bit array (int) to toggle the reporting of the analogIns
 */
//void FirmataClass::setAnalogPinReporting(byte pin, byte state) {
//}
void AnalogInputFirmata::reportAnalog(byte analogPin, int value)
{
  if (analogPin < TOTAL_ANALOG_PINS) {
    // value is the report interval in millis or seconds, depending on the first bit. 0 is millis, 1 is seconds.
    // if value is in millis and <= MINIMUM_SAMPLING_INTERVAL, pin will be reported in the global sampling loop
    analogPinsReportInterval[analogPin] = value;
    // prevent during system reset or all analog pin values will be reported
    // which may report noise for unconnected analog pins
    if (!Firmata.isResetting()) {
      // Send pin value immediately. This is helpful when connected via
      // ethernet, wi-fi or bluetooth so pin states can be known upon
      // reconnecting.
      Firmata.sendAnalog(analogPin, analogRead(analogPin));
      analogPinsPreviousReport[analogPin] = millis();
    }
    else {
      // set last report time to -reporting interval, being unsigned this wil wrap, but so will the check
      analogPinsPreviousReport[analogPin] = -PIN_SAMPLING_INTERVAL_TIME(analogPinsReportInterval[analogPin]);
    }
  }
  // TODO: save status to EEPROM here, if changed
}

boolean AnalogInputFirmata::handlePinMode(byte pin, int mode)
{
  if (IS_PIN_ANALOG(pin)) {
    if (mode == PIN_MODE_ANALOG) {
      reportAnalog(PIN_TO_ANALOG(pin), 1); // turn on reporting
      if (IS_PIN_DIGITAL(pin)) {
        pinMode(PIN_TO_DIGITAL(pin), INPUT); // disable output driver
      }
      return true;
    } else {
      reportAnalog(PIN_TO_ANALOG(pin), 0); // turn off reporting
    }
  }
  return false;
}

void AnalogInputFirmata::handleCapability(byte pin)
{
  if (IS_PIN_ANALOG(pin)) {
    Firmata.write(PIN_MODE_ANALOG);
    Firmata.write(10); // 10 = 10-bit resolution
  }
}

boolean AnalogInputFirmata::handleSysex(byte command, byte argc, byte* argv)
{
  if (command == REPORT_ANALOG_CONFIG) {
    if (argc > 1) {
      int analogPin = argv[0];
      int val = argv[1];
      if (argc > 2) val |= (argv[2] << 7);
      // val is the report interval in millis or seconds, depending on the first bit. 0 is millis, 1 is seconds.
      // if val is in millis and <= MINIMUM_SAMPLING_INTERVAL, pin will be reported in the global sampling loop
      analogPinsReportInterval[analogPin] = val;
      // prevent during system reset or all analog pin values will be reported
      // which may report noise for unconnected analog pins
      // set last report time to -reporting interval, being unsigned this wil wrap, but so will the check
      analogPinsPreviousReport[analogPin] = -PIN_SAMPLING_INTERVAL_TIME(analogPinsReportInterval[analogPin]);
    }
    return true;
  }
  return handleAnalogFirmataSysex(command, argc, argv);
}

void AnalogInputFirmata::reset()
{
  // by default, do not report any analog inputs
  memset(analogPinsReportInterval, 0, TOTAL_ANALOG_PINS * sizeof analogPinsReportInterval[0]);
  memset(analogPinsPreviousReport, 0, TOTAL_ANALOG_PINS * sizeof analogPinsPreviousReport[0]);
}

void AnalogInputFirmata::report()
{
  reportIndividualPins(true);
}

uint16_t smoothedAnalogRead(uint8_t pin, uint8_t num_readings /* = 10 */, uint16_t delay_between_reads_us /* = 300 */)
{
  uint32_t analog_value = 0;

  for (uint8_t i = 0; i < num_readings; i++){
    // 1ms pause adds more stability between reads.
    // Currently, the largest value that will produce an accurate delay is 16383.
    // https://www.arduino.cc/reference/en/language/functions/time/delaymicroseconds/
    delayMicroseconds(delay_between_reads_us);

    // Read light sensor data.
    analog_value += analogRead(pin);
  }

  // Take an average of all the readings.
  return uint16_t(analog_value / num_readings);
}

void AnalogInputFirmata::reportIndividualPins(bool globalReportingTime /* = false */)
{
  byte pin, analogPin;
  /* ANALOGREAD - do all analogReads() at the configured sampling interval */
  unsigned long currentMillis = millis();
  for (pin = 0; pin < TOTAL_PINS; pin++) {
    if (IS_PIN_ANALOG(pin) && Firmata.getPinMode(pin) == PIN_MODE_ANALOG) {
      analogPin = PIN_TO_ANALOG(pin);
      if ((globalReportingTime && (PIN_SAMPLING_INTERVAL_TIME(analogPinsReportInterval[analogPin]) <= MINIMUM_SAMPLING_INTERVAL)) || // analog pin using default sampling interval
          ((currentMillis - analogPinsPreviousReport[analogPin]) > PIN_SAMPLING_INTERVAL_TIME(analogPinsReportInterval[analogPin]))) // analog pin using individual sampling interval
      {
        Firmata.sendAnalog(analogPin, smoothedAnalogRead(analogPin, 10, 100));
        analogPinsPreviousReport[analogPin] += PIN_SAMPLING_INTERVAL_TIME(analogPinsReportInterval[analogPin]);
      }
    }
  }
}
