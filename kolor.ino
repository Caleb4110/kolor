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
// NOTE: param1 and param2 ins are seperate inputs as they 
//       are not physically connected
AnalogIn analogControls[7] = {
  AnalogIn(A0),   // Red
  AnalogIn(A1),   // Green
  AnalogIn(A2),   // Blue
  AnalogIn(A5),   // Param1
  AnalogIn(A3),   // Param1In
  AnalogIn(A10),  // Param2
  AnalogIn(A4),   // Param2In
};

// Switch/Button declarations
Switch sRgbSwitch(15, INPUT_PULLUP);
EventSwitch sEncoderButton(4, INPUT_PULLUP);

// Allow temporaly dithering
#define delay FastLED.delay

// Define leds and matrix here so they can be used globally
CRGB* leds = nullptr;
FastLED_NeoMatrix *matrix = nullptr;

// The shape to use for the 2D shape effect
uint8_t shape = 0;

/*
Keep the following in sync with the number of modes:
- NUM_MODES
- sEncoderTrackers
- Mode enum (in common.h)
- Function pointer array (below sEncoderTrackers declaration)
*/
#define NUM_MODES 9
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
  //EncoderTracker(EEPROM_ENCODER_OFFSET + 9),
};

// Function pointer array
void (*modes[NUM_MODES]) (CRGB *, FastLED_NeoMatrix *, uint16_t, unsigned long, uint8_t, uint8_t[], uint8_t[]) = {
  setup_matrix,
  default_effect,
  //pulse,
  trails,
  rainbow_pulse,
  rainbow_trails,
  chase,
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

//===============COLOUR ORDER HANDLING===============//
String colourOrder = "rgb";

// Used to index into the colour values to retrieve the correct one based on the selected colour order
uint8_t colourIndices[3] = { 0, 1, 2 };

// Function to get the correct parameter for the current colour order
void adjustColourIndices() {
  for (int i = 0; i < 3; i++) {
    char c = tolower(colourOrder[i]);
    if (c == 'r')      colourIndices[i] = Red;
    else if (c == 'g') colourIndices[i] = Green;
    else if (c == 'b') colourIndices[i] = Blue;
    // If the format isn't valid, set back to default and break
    else {
      colourIndices[0] = Red;
      colourIndices[1] = Green;
      colourIndices[2] = Blue;
      break;
    }
  }
}

// Returns true if a restart is required
bool parseInput(char *input) {
  bool shouldRestart = false;

  // First pass split the incoming data by the comma "," character
  char *tempData[128] = {0};
  char *token = strtok(input, ",");
  int count = 1;
  while(token != NULL) {
    tempData[count-1] = token;
    token = strtok(NULL, ",");
    count++;
  }
  for (int i = 0; i < count; i++) {
    char *entry = tempData[i];
    char *key = strtok(entry, ":");
    char *value = strtok(NULL, ":");

    if (key && value) {
      if (strcmp(key, "colourOrder") == 0) {
        colourOrder = value;
      }
      else if (strcmp(key, "width") == 0) {
        new_width = (uint16_t) strtoul(value, NULL, 10);
        shouldRestart = true;
      }
      else if (strcmp(key, "height") == 0) {
        new_height = (uint16_t) strtoul(value, NULL, 10);
        shouldRestart = true;
      }
      else if (strcmp(key, "posVertical") == 0) {
        matrix_pos_v = (uint8_t) strtoul(value, NULL, 10);
        shouldRestart = true;
      }
      else if (strcmp(key, "posHorizontal") == 0) {
        matrix_pos_h = (uint8_t) strtoul(value, NULL, 10);
        shouldRestart = true;
      }
      else if (strcmp(key, "orientation") == 0) {
        matrix_orientation = (uint8_t) strtoul(value, NULL, 10);
        shouldRestart = true;
      }
      else if (strcmp(key, "layout") == 0) {
        matrix_layout = (uint8_t) strtoul(value, NULL, 10);
        shouldRestart = true;
      }
      else if (strcmp(key, "shape") == 0) {
        shape = (uint8_t) strtoul(value, NULL, 10);
        EEPROM.write(EEPROM_MATRIX_OFFSET + Shape, shape);
      }
    }
  }
  return shouldRestart;
}

//===============COLOUR ORDER HANDLING===============//

// Function to write all matrix values to the EEPROM at once
void writeMatrixValuesToEEPROM(uint8_t width, uint8_t height, uint8_t pos_v, uint8_t pos_h, uint8_t orientation, uint8_t layout) {
  EEPROM.write(EEPROM_MATRIX_OFFSET + MatrixWidth, width);
  EEPROM.write(EEPROM_MATRIX_OFFSET + MatrixHeight, height);
  EEPROM.write(EEPROM_MATRIX_OFFSET + PosV, pos_v);
  EEPROM.write(EEPROM_MATRIX_OFFSET + PosH, pos_h);
  EEPROM.write(EEPROM_MATRIX_OFFSET + Orient, orientation);
  EEPROM.write(EEPROM_MATRIX_OFFSET + Layout, layout);
}

void writeAndReset(uint8_t width, uint8_t height, uint8_t pos_v, uint8_t pos_h, uint8_t orientation, uint8_t layout) {
  // Write all values to eeprom
  writeMatrixValuesToEEPROM(  
    width,    height,         pos_v, 
    pos_h,    orientation,    layout
  );

  // Save the new width and height to the encoder
  sEncoderTrackers[SetupMatrix]
    .storeToEncoder(new_width, new_height);

  // Light up panel to indicate a reset
  FastLED.clear();
  fill_rainbow_circular(
    leds, new_height * new_width, 
    127,  true
  );

  // Reset the system
  delay(250);
  digitalWrite(0, LOW);
}

void setup() {
  Serial.begin(9600);            // Open the USB Serial Port
  
  digitalWrite(RESET_PIN, HIGH); // Write high to the reset pin...
  pinMode(RESET_PIN, OUTPUT);    // BEFORE setting it to output

  sEncoderButton.setup();        // Setup encoder
  sRgbSwitch.setup();            // Setup switch

  for (uint8_t i = 0; i < NUM_MODES; i++) 
    sEncoderTrackers[i].setup(); // Setup each encoder tracker

  //===================MATRIX SETUP====================//
  // Read all EEPROM values to local variables
  uint8_t eeprom_vals[EEPROM_MATRIX_NUM_FIELDS];
  for (uint8_t i = 0; i < EEPROM_MATRIX_NUM_FIELDS; i++) {
    eeprom_vals[i] = EEPROM.read(EEPROM_MATRIX_OFFSET + i);
  }

  uint16_t eeprom_width = eeprom_vals[MatrixWidth];
  uint16_t eeprom_height = eeprom_vals[MatrixHeight];

  // Matrix size, orientation, layout
  // If the eeprom width or height are valid then use them
  if ((eeprom_width > 0 && eeprom_width <= MAX_MATRIX_SIZE) && 
      (eeprom_height > 0 && eeprom_height <= MAX_MATRIX_SIZE) && 
      (eeprom_width * eeprom_height) <= MAX_MATRIX_SIZE) {
    matrix_width = eeprom_width;
    matrix_height = eeprom_height;
    matrix_size = matrix_width * matrix_height;
    sEncoderTrackers[SetupMatrix].storeToEncoder(matrix_width, matrix_height);
  }

  // Shape for shape effect
  uint8_t eeprom_shape = eeprom_vals[Shape];
  if (eeprom_shape > 0 && eeprom_shape <= 2) shape = eeprom_shape;

  // If the orientation, layout or pixel positions are valid, then use them
  matrix_pos_v = (eeprom_vals[PosV] == NEO_MATRIX_TOP || eeprom_vals[PosV] == NEO_MATRIX_BOTTOM) ? eeprom_vals[PosV] : matrix_pos_v;
  matrix_pos_h = (eeprom_vals[PosH] == NEO_MATRIX_LEFT || eeprom_vals[PosH] == NEO_MATRIX_RIGHT) ? eeprom_vals[PosH] : matrix_pos_h;
  matrix_orientation = (eeprom_vals[Orient] == NEO_MATRIX_ROWS || eeprom_vals[Orient] == NEO_MATRIX_COLUMNS) ? eeprom_vals[Orient] : matrix_orientation;
  matrix_layout = (eeprom_vals[Layout] == NEO_MATRIX_ZIGZAG || eeprom_vals[Layout] == NEO_MATRIX_PROGRESSIVE) ? eeprom_vals[Layout] : matrix_layout;

  // Set the leds to the matrix size and add to fastLED (for 1D) and NeoMatrix (for 2D)
  leds = new CRGB[matrix_size];
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, matrix_size);
  matrix = new FastLED_NeoMatrix(leds, matrix_width, matrix_height, 1, 1, 
    matrix_pos_v + matrix_pos_h +
    matrix_orientation + matrix_layout);
  //===================MATRIX SETUP====================//

  //=====================MODE SETUP====================//
  // Use the last stored mode if it's a valid mode
  uint8_t modeFromEEPROM = EEPROM.read(EEPROM_CURRENT_EFFECT);
  if (modeFromEEPROM > 0 && modeFromEEPROM < NUM_MODES) {
    // It's a valid mode and can be used
    sMode = (Mode) modeFromEEPROM;
  }
  //=====================MODE SETUP====================//
}

void loop() {
  // Handles rgb changing
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim(); // remove whitespace and \r characters
    if (input.length() > 0) {
        if (parseInput(input.c_str())) {
          writeAndReset(new_width,    new_height,         matrix_pos_v, 
                        matrix_pos_h, matrix_orientation, matrix_layout);
        }
        adjustColourIndices();
    }
  }

  //==================REFRESH CONTROLS=================//
  for (size_t i = 0; i < NumAnalogControls; i++) {
    analogControls[i].refresh();
  }

  sRgbSwitch.refresh();
  sEncoderTrackers[sMode].update(sEncoder.readAndReset(), sRgbSwitch.value(), sMode);

  Event event = sEncoderButton.poll_for_events();
  //==================REFRESH CONTROLS=================//

  //==================HANDLE THE EVENT=================//
  switch (event) {
    case NoEvent: // Do nothing
      break;
    case Click:
      if (sMode == SetupMatrix) { // Return to the last mode
        sMode = sLastNonSetupMode;

        writeAndReset(
          new_width,
          new_height,         
          matrix_pos_v < (255 >> 1) ? NEO_MATRIX_TOP : NEO_MATRIX_BOTTOM, 
          matrix_pos_h < (255 >> 1) ? NEO_MATRIX_LEFT : NEO_MATRIX_RIGHT, 
          matrix_orientation < (255 >> 1) ? NEO_MATRIX_ROWS : NEO_MATRIX_COLUMNS, 
          matrix_layout < (255 >> 1) ? NEO_MATRIX_ZIGZAG : NEO_MATRIX_PROGRESSIVE
        );

      } else { // Go to next mode
        // Next non-setup mode
        sMode = (Mode) ((sMode + 1) % NUM_MODES);
        if (sMode == SetupMatrix) {
          // Go to next non-setup mode
          sMode = (Mode) (sMode + 1);
        }

        // Save to the eeprom
        EEPROM.write(EEPROM_CURRENT_EFFECT, sMode);
      }
      sFirstTick = true;
      break;
    case LongPress:
      if (sMode == SetupMatrix) { // Cancel setup mode
        sMode = sLastNonSetupMode;
        sFirstTick = true;
      } else { // Enter setup mode
        sLastNonSetupMode = sMode;
        sMode = SetupMatrix;
        sFirstTick = true;
      }
      break;
  }
  //==================HANDLE THE EVENT=================//

  // Get the current value for the encoder based on the current mode
  uint8_t effect_param = sEncoderTrackers[sMode].value();

  // Handles the case that we are in setup mode
  if (sMode == SetupMatrix) {
    // If the switch is toggled, set the effect param to the height value
    if (sRgbSwitch.value()) { 
      effect_param = sEncoderTrackers[sMode].valueHeight();
    }
    new_width = sEncoderTrackers[SetupMatrix].value() + 1;
    new_height = sEncoderTrackers[SetupMatrix].valueHeight() + 1;
    matrix_pos_v = analogControls[Red].value();
    matrix_pos_h = analogControls[Green].value();
    matrix_orientation = analogControls[Blue].value();
    matrix_layout = analogControls[Param1].value();
  }
  
  // Timing
  unsigned long clock = 0;
  if (sFirstTick) {
    sClockRef = millis();
  } else {
    clock = millis() - sClockRef;
  }

  // Combine all control values into one array
  uint8_t controlVals[] = { 
    analogControls[colourIndices[0]].value(), 
    analogControls[colourIndices[1]].value(), 
    analogControls[colourIndices[2]].value(), 
    analogControls[Param1].value(), 
    analogControls[Param1In].value(), 
    analogControls[Param2].value(), 
    analogControls[Param2In].value(),
    sRgbSwitch.value(),
    effect_param,
  };

  // Combine all matrix values into one array
  uint8_t matrixVals[] = { 
    matrix_width, 
    matrix_height, 
    matrix_pos_v, 
    matrix_pos_h, 
    matrix_orientation, 
    matrix_layout,
    shape // I know that the shape isn't actually a matrix val, but its saved in memory with the matrix values
  };

  // Run current mode and pass the control values and matrix values as arguments
  modes[ sMode ](leds, matrix, matrix_size, clock, sFirstTick, controlVals, matrixVals);
  
  sFirstTick = false;
}
