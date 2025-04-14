// LED libraries for 1d and 2d drawing
#include <FastLED.h>
#include <Adafruit_GFX.h>
#include <FastLED_NeoMatrix.h>

// Encoder and EEPROM libraries
#include <Encoder.h>
#include <EEPROM.h>

// Other
#include "common.h"
#include "elkIO.h"
#include "effects.h"

// The rotary encoder
Encoder sEncoder(2, 3);

// Pin number for LED strip and reset pin
#define LED_PIN 14
#define RESET_PIN 0

// Potentiometer declarations
// NOTE: Change/add the second value if knob is jittery
AnalogIn sRed(A0);
AnalogIn sGreen(A1);
AnalogIn sBlue(A2);
AnalogIn sParam1(A5);
AnalogIn sParam2(A10);

// Input sockets declarations
// NOTE: Only param1 and param2 inputs as these
// are the only ones not physically connected
AnalogIn sParam1In(A3);
AnalogIn sParam2In(A4);

// Switch/Button declarations
Switch sRgbSwitch(15, INPUT_PULLUP);
EventSwitch sEncoderButton(4, INPUT_PULLUP);

// Allow temporaly dithering
#define delay FastLED.delay

// Define leds and matrix here so they can be used globally
CRGB* leds = nullptr;
FastLED_NeoMatrix *matrix = nullptr;

/*
Keep the following in sync with the number of modes:
- NUM_MODES
- sEncoderTrackers
- Mode enum (in common.h)
- Function pointer array (below sEncoderTrackers declaration)
*/
#define NUM_MODES 12
EncoderTracker sEncoderTrackers[NUM_MODES] = {
  EncoderTracker(EEPROM_MATRIX_OFFSET),
  EncoderTracker(EEPROM_ENCODER_OFFSET + 1),
  EncoderTracker(EEPROM_ENCODER_OFFSET + 2),
  EncoderTracker(EEPROM_ENCODER_OFFSET + 3),
  EncoderTracker(EEPROM_ENCODER_OFFSET + 4),
  EncoderTracker(EEPROM_ENCODER_OFFSET + 5),
  EncoderTracker(EEPROM_ENCODER_OFFSET + 6),
  EncoderTracker(EEPROM_ENCODER_OFFSET + 7),
  EncoderTracker(EEPROM_ENCODER_OFFSET + 8),
  EncoderTracker(EEPROM_ENCODER_OFFSET + 9),
  EncoderTracker(EEPROM_ENCODER_OFFSET + 10),
  EncoderTracker(EEPROM_ENCODER_OFFSET + 11),
};

// Function pointer array
void (*modes[NUM_MODES]) (CRGB *, FastLED_NeoMatrix *, uint16_t, unsigned long, uint8_t, uint8_t[], uint8_t[]) = {
  setup_matrix,
  default_effect,
  pulse,
  trails,
  rainbow,
  rainbow_pulse,
  rainbow_trails,
  chase,
  bounce,
  sparkle,
  circles_2d,
  ascii_2d,
};

// Mode controls both:
// - The current effect AND the current 
//   encoder value for said effect
// Default value for the current mode is the default effect
Mode sMode = DefaultEffect;
Mode sLastNonSetupMode = DefaultEffect;

// Default matrix values 
// NOTE: This is the default for the 8x8 panel sent out with most KOLORS
uint16_t matrix_height = 8; // Default matrix height
uint16_t matrix_width = 8; // Default matrix length
uint8_t matrix_pos_v = NEO_MATRIX_TOP; // Default value for vertical start pos
uint8_t matrix_pos_h = NEO_MATRIX_RIGHT; // Default value for horizontal start pos
uint8_t matrix_orientation = NEO_MATRIX_COLUMNS; // Default value for orientation
uint8_t matrix_layout = NEO_MATRIX_PROGRESSIVE; // Default value for layout
uint16_t matrix_size = matrix_height * matrix_width; // Default matrix size

// These are used by the setup mode to store the new width and height values
uint16_t new_height;
uint16_t new_width;

// Set to 'true' on the first render after a mode switch
bool sFirstTick = false;

// Starting reference for the clock
unsigned long sClockRef = 0;

// Function to refresh all parameters at once
Event refreshAll() {
  sRed.refresh();
  sGreen.refresh();
  sBlue.refresh();
  sParam1.refresh();
  sParam1In.refresh();
  sParam2.refresh();
  sParam2In.refresh();
  sRgbSwitch.refresh();
  sEncoderTrackers[sMode].update(sEncoder.readAndReset(), sRgbSwitch.value(), sMode);

  return sEncoderButton.poll_for_events();
}

// Function to write all matrix values to the EEPROM at once
void writeMatrixValuesToEEPROM(uint8_t width, uint8_t height, uint8_t pos_v, uint8_t pos_h, uint8_t orientation, uint8_t layout) {
  EEPROM.write(EEPROM_MATRIX_OFFSET + MatrixWidth, width);
  EEPROM.write(EEPROM_MATRIX_OFFSET + MatrixHeight, height);
  EEPROM.write(EEPROM_MATRIX_OFFSET + PosV, pos_v < (255 >> 1) ? NEO_MATRIX_TOP : NEO_MATRIX_BOTTOM);
  EEPROM.write(EEPROM_MATRIX_OFFSET + PosH, pos_h < (255 >> 1) ? NEO_MATRIX_LEFT : NEO_MATRIX_RIGHT);
  EEPROM.write(EEPROM_MATRIX_OFFSET + Orient, orientation < (255 >> 1) ? NEO_MATRIX_ROWS : NEO_MATRIX_COLUMNS);
  EEPROM.write(EEPROM_MATRIX_OFFSET + Layout, layout < (255 >> 1) ? NEO_MATRIX_ZIGZAG : NEO_MATRIX_PROGRESSIVE);
}

void setup() {
  // Open the USB Serial Port
  Serial.begin(9600);

  // Write high to the reset pin BEFORE setting it to output
  // So we don't enter a reset loop
  digitalWrite(RESET_PIN, HIGH);
  pinMode(RESET_PIN, OUTPUT);

  // Setup switch and encoder
  sEncoderButton.setup();
  sRgbSwitch.setup();

  // Setup each encoder tracker
  for (uint8_t i = 0; i < NUM_MODES; i++) {
    sEncoderTrackers[i].setup();
  }

  // Fetch width, height and num_leds from eeprom
  uint16_t eeprom_width = EEPROM.read(EEPROM_MATRIX_OFFSET + MatrixWidth);
  uint16_t eeprom_height = EEPROM.read(EEPROM_MATRIX_OFFSET + MatrixHeight);
  uint8_t eeprom_pos_v = EEPROM.read(EEPROM_MATRIX_OFFSET + PosV);
  uint8_t eeprom_pos_h = EEPROM.read(EEPROM_MATRIX_OFFSET + PosH);
  uint8_t eeprom_orientation = EEPROM.read(EEPROM_MATRIX_OFFSET + Orient);
  uint8_t eeprom_layout = EEPROM.read(EEPROM_MATRIX_OFFSET + Layout);

  // Use the stored matrix width and height if they are in the correct range
  if (
    eeprom_width > 0 && eeprom_width <= MAX_MATRIX_SIZE &&
    eeprom_height > 0 && eeprom_height <= MAX_MATRIX_SIZE &&
    (eeprom_width * eeprom_height <= MAX_MATRIX_SIZE)
  ) {
    matrix_height = eeprom_height;
    matrix_width = eeprom_width;
    matrix_size = eeprom_height * eeprom_width;
    sEncoderTrackers[SetupMatrix].storeToEncoder(matrix_width, matrix_height);
  }

  // Use the stored vertical start position if it's a valid position
  if (eeprom_pos_v == NEO_MATRIX_TOP || eeprom_pos_v == NEO_MATRIX_BOTTOM) {
    matrix_pos_v = eeprom_pos_v;
  }

  // Use the stored horizontal start position if it's a valid position
  if (eeprom_pos_h == NEO_MATRIX_LEFT || eeprom_pos_h == NEO_MATRIX_RIGHT) {
    matrix_pos_h = eeprom_pos_h;
  }

  // Use the stored matrix orientation if it's a valid orientation
  if (eeprom_orientation == NEO_MATRIX_ROWS || eeprom_orientation == NEO_MATRIX_COLUMNS) {
    matrix_orientation = eeprom_orientation;
  }

  // Use the stored matrix layout if it's a valid layout
  if (eeprom_layout == NEO_MATRIX_ZIGZAG || eeprom_layout == NEO_MATRIX_PROGRESSIVE) {
    matrix_layout = eeprom_layout;
  }

  // Use the last stored mode if it's a valid mode
  uint8_t modeFromEEPROM = EEPROM.read(EEPROM_CURRENT_EFFECT);
  if (modeFromEEPROM > 0 && modeFromEEPROM < NUM_MODES) {
    // It's a valid mode and can be used
    sMode = (Mode) modeFromEEPROM;
  }
  
  // Set the leds to the matrix size and add to fastLED (for 1D) and NeoMatrix (for 2D)
  leds = new CRGB[matrix_size];
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, matrix_size);
  matrix = new FastLED_NeoMatrix(leds, matrix_width, matrix_height, 1, 1, 
    matrix_pos_v + matrix_pos_h +
    matrix_orientation + matrix_layout);
}

void loop() {
  Event event = refreshAll();

  switch (event) {
    case NoEvent:
      // do nothing
      break;
    case Click:
      if (sMode == SetupMatrix) {
        // Return to last used mode
        sMode = sLastNonSetupMode;

        // Write all values to eeprom
        writeMatrixValuesToEEPROM(new_width, new_height, matrix_pos_v, matrix_pos_h, matrix_orientation, matrix_layout);
        sEncoderTrackers[SetupMatrix].storeToEncoder(new_width, new_height);

        // Set new size of strip/panel rainbow
        Serial.println("Resetting...");
        FastLED.clear();
        fill_rainbow_circular(leds, new_height * new_width, 127, true);

        // Reset the system
        delay(500);
        digitalWrite(0, LOW);
      } else {
        // next non-setup mode
        sMode = (Mode) ((sMode + 1) % NUM_MODES);
        if (sMode == SetupMatrix) {
          // Go to next non-setup mode
          sMode = (Mode) (sMode + 1);
        }
        
        Serial.print("switch to mode ");
        Serial.println(sMode);
        EEPROM.write(EEPROM_CURRENT_EFFECT, sMode);
      }
      sFirstTick = true;
      break;
    case LongPress:
      if (sMode == SetupMatrix) {
        // Return to last used mode without setting new matrix values
        sMode = sLastNonSetupMode;
        sFirstTick = true;
      } else {
        // Enter setup mode
        sLastNonSetupMode = sMode;
        sMode = SetupMatrix;
        sFirstTick = true;
      }
      break;
  }

  // Get the current value for the encoder based on the current mode
  uint8_t effect_param = sEncoderTrackers[sMode].value();

  if (sMode == SetupMatrix) {
    if (sRgbSwitch.value()) { // changes the effect parameter to height if in height position
      effect_param = sEncoderTrackers[sMode].valueHeight();
    }
    new_width = sEncoderTrackers[SetupMatrix].value() + 1;
    new_height = sEncoderTrackers[SetupMatrix].valueHeight() + 1;
    matrix_pos_v = sRed.value();
    matrix_pos_h = sGreen.value();
    matrix_orientation = sBlue.value();
    matrix_layout = sParam1.value();
  }
  
  unsigned long clock = 0;
  if (sFirstTick) {
    sClockRef = millis();
  } else {
    clock = millis() - sClockRef;
  }

  // Combine all control values into one array
  uint8_t controlVals[] = { 
    sRgbSwitch.value(), 
    sRed.value(), 
    sGreen.value(), 
    sBlue.value(), 
    sParam1.value(), 
    sParam1In.value(), 
    sParam2.value(), 
    sParam2In.value(),
    effect_param
  };

  // Combine all matrix values into one array
  uint8_t matrixVals[] = { 
    matrix_width, 
    matrix_height, 
    matrix_pos_v, 
    matrix_pos_h, 
    matrix_orientation, 
    matrix_layout
  };

  // Run current mode and pass the control values and matrix values as arguments
  modes[ sMode ](leds, matrix, matrix_size, clock, sFirstTick, controlVals, matrixVals);
  
  sFirstTick = false;
}
