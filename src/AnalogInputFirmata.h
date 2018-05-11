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
*/

#ifndef AnalogInputFirmata_h
#define AnalogInputFirmata_h

#include <ConfigurableFirmata.h>
#include "FirmataFeature.h"
#include "FirmataReporting.h"

// pin sampling interval has 14 bits
// first bit is time unit (0 = ms, 1 = s)
#define PIN_SAMPLING_INTERVAL_TIME_UNIT_MASK 0x2000
// 13 other bits is the actual interval
#define PIN_SAMPLING_INTERVAL_VALUE_MASK     0x1FFF
#define PIN_SAMPLING_INTERVAL_TIME(v) ((v & PIN_SAMPLING_INTERVAL_TIME_UNIT_MASK) ? (v & PIN_SAMPLING_INTERVAL_VALUE_MASK) * 1000 : (v & PIN_SAMPLING_INTERVAL_VALUE_MASK))

void reportAnalogInputCallback(byte analogPin, int value);

class AnalogInputFirmata: public FirmataFeature
{
  public:
    AnalogInputFirmata();
    void reportAnalog(byte analogPin, int value);
    void handleCapability(byte pin);
    boolean handlePinMode(byte pin, int mode);
    boolean handleSysex(byte command, byte argc, byte* argv);
    void reset();
    void report();
    void reportIndividualPins(bool globalReportingTime = false);

  private:
    /* analog inputs */
    unsigned int analogPinsReportInterval[TOTAL_ANALOG_PINS]; // most significant bit stores unit (0=millis, 1=seconds)
    unsigned long analogPinsPreviousReport[TOTAL_ANALOG_PINS];
};

#endif
