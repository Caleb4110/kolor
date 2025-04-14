#ifndef ELKIO_H
#define ELKIO_H

#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>
#include "common.h"

class AnalogIn {
  private:
    uint8_t _pin;
    uint8_t _jitter_control_threshold;
    uint8_t _value;
    uint16_t _last_emitted_raw_value;
  public:
    AnalogIn(uint8_t io_pin);
    // Set jitter_control_threshold higher if the knob is jittery-er, at the expense of maybe not being able to pick up all values.
    AnalogIn(uint8_t io_pin, uint8_t jitter_control_threshold);
    void refresh();
    uint8_t value();
};

class Switch {
  private:
    uint8_t _pin;
    uint8_t _pin_mode;
    bool _value;
  public:
    Switch(uint8_t io_pin, uint8_t pin_mode);
    void setup();
    void refresh();
    bool value();
};

enum Event {
  NoEvent,
  Click,
  LongPress,
};

class EventSwitch {
  private:
    Switch _switch;
    bool _last_value;
    // this is the response from millis()
    unsigned long _press_start_time_ms;
    bool _emitted_long_press;
  public:
    EventSwitch(uint8_t io_pin, uint8_t pin_mode);
    void setup();
    Event poll_for_events();
};

class EncoderTracker {
  private:
    int32_t _raw_value;
    int32_t _raw_value_height; // Used by Setup 
    // might regret having this as a uint8_t because it pretty dramatically limits our options, but
    // it makes it easy to store / retrieve from EEPROM
    uint8_t _output_value;
    uint8_t _output_value_height;
    // set to zero to skip EEPROM
    uint8_t _eeprom_address;

  public:
    EncoderTracker(uint8_t eeprom_address);
    void update(int32_t increment, uint8_t switched, Mode currentMode);
    void storeToEncoder(uint8_t width, uint8_t height); // ensures width and height load the same on the setup function
    uint8_t value();
    uint8_t valueHeight();
    void setup();
};

#endif // ELKIO_H
