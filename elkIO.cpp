#include "elkIO.h"

#define DEFAULT_JITTER_CONTROL_THRESHOLD 2

//===========================================AnalogIn===========================================
AnalogIn::AnalogIn(uint8_t io_pin) : AnalogIn(io_pin, DEFAULT_JITTER_CONTROL_THRESHOLD) {}

AnalogIn::AnalogIn(uint8_t io_pin, uint8_t jitter_control_threshold) {
  _pin = io_pin;
  _jitter_control_threshold = jitter_control_threshold;
}

uint8_t AnalogIn::value() {
  return _value;
}

uint8_t map_value(uint16_t raw_analog_value) {
  // there's an issue with the voltages on the pots which mean they don't use the full range and 
  // clip at about 1000 out of the DAC (instead of 1024) so we mult by (256 / 1000), which requires
  // converting the type up. The division by 1000 is probably real expensive but it's hopefully ok
  // because we only run this function whenever the value actually changes because of the anti-
  // jitter logic.
  if (raw_analog_value >= 1000) {
    return 255;
  }
  return ((uint32_t) raw_analog_value << 8) / 1000;
}

void AnalogIn::refresh() {
  uint16_t new_raw_value = analogRead(_pin);
  int32_t diff = (int32_t) new_raw_value - _last_emitted_raw_value;
  if (abs(diff) > _jitter_control_threshold) {
    _value = map_value(new_raw_value);
    _last_emitted_raw_value = new_raw_value;
  }
}
//==============================================================================================


//============================================Switch============================================
Switch::Switch(uint8_t io_pin, uint8_t pin_mode){
  _pin = io_pin;
  _pin_mode = pin_mode;
}

void Switch::setup() {
  pinMode(_pin, _pin_mode);
}

void Switch::refresh() {
  _value = digitalRead(_pin);
}

bool Switch::value() {
  return _value;
}
//==============================================================================================


//==========================================EventSwitch=========================================
EventSwitch::EventSwitch(uint8_t io_pin, uint8_t pin_mode): _switch(io_pin, pin_mode) {
  // probably need to do something clever based on pin_mode and default values here or pass a param to 
  // say which is pressed and not pressed
  // Set _last_value to true so you don't get a phantom click on boot
  _last_value = true;
}

void EventSwitch::setup() {
  _switch.setup();
}

Event EventSwitch::poll_for_events() {
  _switch.refresh();
  Event event = NoEvent;
  
  if (_last_value != _switch.value()) {
    if (_switch.value()) {
      // released
      if (!_emitted_long_press) {
        event = Click;
      }
    } else {
      // pressed
      _press_start_time_ms = millis();
      _emitted_long_press = false;
    }
    _last_value = _switch.value();
  }

  if (
    // currently pressed
    !_switch.value() && 
    // first pressed > 1000 millis
    (millis() - _press_start_time_ms) > 1000 && 
    // not yet emitted
    !_emitted_long_press) {
      _emitted_long_press = true;
      event = LongPress;
  }
  return event;
}
//==============================================================================================


//=========================================EncoderTracker=======================================
EncoderTracker::EncoderTracker(uint8_t eeprom_address) {
  _eeprom_address = eeprom_address;
}

void EncoderTracker::update(int32_t increment, uint8_t switched, Mode currentMode) {
  // if the toggle switch is set to height, we check the height variables and write with offset
  if (switched && currentMode == SetupMatrix) {
    uint8_t old_val = (uint8_t)(_raw_value_height >> 2);
    _raw_value_height += increment;
    if (_raw_value_height < 0) {
      _raw_value_height = 0;
    } else if (_raw_value_height > (0xFF << 2)) {
      _raw_value_height = (0xFF << 2);
    }
    _output_value_height = (_raw_value_height >> 2);
    if (_output_value_height != old_val) {
      EEPROM.write(_eeprom_address + 1, _output_value_height);
    }
  } else {
    uint8_t old_val = (uint8_t)(_raw_value >> 2);
    _raw_value += increment;
    if (_raw_value < 0) {
      _raw_value = 0;
    } else if (_raw_value > (0xFF << 2)) {
      _raw_value = (0xFF << 2);
    }
    _output_value = (_raw_value >> 2);
    if (_output_value != old_val) {
      EEPROM.write(_eeprom_address, _output_value);
    }
  }
}

void EncoderTracker::storeToEncoder(uint8_t width, uint8_t height) {
  _output_value = width-1;
  _output_value_height = height-1;
  _raw_value = (width-1 << 2);
  _raw_value_height = (height-1 << 2);
}

uint8_t EncoderTracker::value() {
  return _output_value;
}

uint8_t EncoderTracker::valueHeight() {
  return _output_value_height;
}

void EncoderTracker::setup() {
  _output_value = EEPROM.read(_eeprom_address);
  // if EEPROM read tells us that there's no value
  if (_output_value == 0xFF) {
    _output_value = 0;
  }
  _raw_value = (_output_value << 2);
}
//==============================================================================================
