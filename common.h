#ifndef COMMON_H
#define COMMON_H

// Used to access the the analog controls array
// And also used to access values in effect functions
enum AnalogControls {
  Red,
  Green,
  Blue,
  Param1,
  Param1In,
  Param2,
  Param2In,
  NumAnalogControls
};

// Used to access values in effect functions
enum OtherControls {
  RgbSwitch = 7,
  EncoderVal = 8,
  NumOtherControls = 2,
};

// Used to access the matrix values array given to effect functions
enum Matrix {
  MatrixWidth,
  MatrixHeight,
  PosV,
  PosH,
  Orient,
  Layout,
  Shape
};

// Enum for each effect mode
enum Mode {
  SetupMatrix,
  DefaultEffect,
  //Pulse,
  Trails,
  RainbowPulse,
  RainbowTrails,
  Chase,
  Sparkle,
  Circles2d,
  Ascii2d,
};

// The maximum number of LEDS a strip/ matrix can have
#define MAX_MATRIX_SIZE 256

// EEPROM data offsets //
#define EEPROM_CURRENT_EFFECT 0
#define EEPROM_ENCODER_OFFSET 1

/*
 * KOLOR saves the matrix values in eeprom for persistent size and orientation on reboots
 * Saved as:
 * 60: width
 * 61: height
 * 62: top/bottom
 * 63: left/right
 * 64: row/column
 * 65: zigzag/progressive
 * 66: shape
 */
#define EEPROM_MATRIX_OFFSET 60
#define EEPROM_MATRIX_NUM_FIELDS 7

#endif
