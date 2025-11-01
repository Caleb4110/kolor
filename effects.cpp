#include "crgb.h"
#include "FastLED.h"
#include "effects.h"

const CRGB colours[4][2] PROGMEM = {
  { CRGB(50, 0, 0),    CRGB(50, 50, 0) },
  { CRGB(0, 50, 0),    CRGB(0, 50, 50) },
  { CRGB(0, 0, 50),    CRGB(50, 0, 50) },
  { CRGB(50, 10, 20),  CRGB(50, 10, 0) }
};

//===================================UTILITIES=====================================//
// Color function for FastLED library
inline CRGB color(bool use_rgb, uint8_t r_or_h, uint8_t g_or_s, uint8_t b_or_v) {
  return use_rgb ? CRGB(r_or_h, g_or_s, b_or_v) : CHSV(r_or_h, g_or_s, b_or_v);
}

void clamp_led_block(uint16_t &width, uint16_t mid_point, uint16_t matrixSize, uint8_t &start_pixel) {
    uint8_t half_width = width >> 1;
    if (half_width > mid_point) {
        start_pixel = 0;
        width -= half_width - mid_point;
    } else {
        start_pixel = mid_point - half_width;
    }
    if (start_pixel + width >= matrixSize) {
        width = matrixSize - start_pixel;
    }
}

// This allows param1 and param2 pots to be digitally linked with their adjacent inputs,
// meaning they can function the same as the Red Green & Blue pots and inputs.
uint8_t computeOutputValue(uint8_t potValue, uint8_t inputValue) {
  // Scale inputValue from range [0, 255] to [-1.0, 1.0]
    float scale = (inputValue - 127.0f) / 127.0f;

    // Compute new output by shifting around potValue
    int output = potValue + scale * (255 - potValue);

    // Clamp between 0 and 255
    if (output < 0) output = 0;
    if (output > 255) output = 255;

    return (uint8_t)output;
}

bool fade_leds(CRGB* leds, uint16_t matrixSize, uint8_t fade_increment, unsigned long& last_fade, unsigned long clock, uint8_t fade_time) {
    if ((clock - last_fade) >= fade_time) {
        last_fade = clock;
        fadeToBlackBy(leds, matrixSize, fade_increment);
        return true;
    }
    return false;
}

//====================================SETTINGS=====================================//
// Setup number of pixels for strip 
void setup_matrix(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  FastLED.clear();

  static unsigned long last_change;
  static uint8_t lastVals[4];
  static CRGB currentColour;

  CRGB cl = CRGB(0, 127, 127);
  CRGB cr = CRGB(127, 127, 0);

  if (first_tick) {
    last_change = 0;
    for (uint8_t i = 1; i < 5; i++) {
      lastVals[i-1] = controlVals[i] < (255 >> 1) ? 0 : 1;
    }
  }

  if (clock - last_change < 1000) {
    fill_solid(&leds[0], matrixSize, currentColour);
  }

  // Check if a parameter has changed and break if found
  for (uint8_t i = 0; i < 4; i++) {
    const uint8_t controlVal = controlVals[i] < (255 >> 1) ? 0 : 1;
    if (lastVals[i] != controlVal) {
      lastVals[i] = controlVal;
      memcpy_P(&currentColour, &colours[i][controlVal], sizeof(CRGB));
      last_change = clock;
      break;
    }
  }


  // Handles height display
  if (controlVals[RgbSwitch]) {
    // If only one pixel, change the color to red
    if (controlVals[EncoderVal] == 0) {
      matrix->drawLine(0, controlVals[EncoderVal], 0, 0, matrix->Color(255, 0, 0));
      matrix->show();
    } 
    // Otherwise, color as the height, increasing green value with each layer
    else {
      if (controlVals[EncoderVal] >= matrixVals[MatrixHeight]) {
        uint8_t remaining_pixels = controlVals[EncoderVal] % matrixVals[MatrixHeight];
        uint8_t layer_number = controlVals[EncoderVal] / matrixVals[MatrixHeight];
        uint8_t prev_layer = layer_number == 0 ? 0 : layer_number - 1;

        matrix->drawLine(0, 0, 0, remaining_pixels, matrix->Color(0, 50*layer_number, 150));
        matrix->show();
        matrix->drawLine(0, remaining_pixels + 1, 0, matrixVals[MatrixHeight], matrix->Color(0, 50*prev_layer, 150));
        matrix->show();
      } else {
        matrix->drawLine(0, controlVals[EncoderVal], 0, 0, matrix->Color(0, 0, 150));
        matrix->show();
      }
    }
  }
  // Handles width display
  else {
    // If only one pixel, change the color to red
    if (controlVals[EncoderVal] == 0) {
      matrix->drawLine(0, 0, controlVals[EncoderVal], 0, matrix->Color(255, 0, 0));
      matrix->show();
    } 
    // Otherwise, color as the width, increasing blue value with each layer
    else {
      if (controlVals[EncoderVal] >= matrixVals[MatrixWidth]) {
        uint8_t remaining_pixels = controlVals[EncoderVal] % matrixVals[MatrixWidth];
        uint8_t layer_number = controlVals[EncoderVal] / matrixVals[MatrixWidth];
        uint8_t prev_layer = layer_number == 0 ? 0 : layer_number - 1;

        matrix->drawLine(0, 0, remaining_pixels, 0, matrix->Color(0, 150, 50*layer_number));
        matrix->show();
        matrix->drawLine(remaining_pixels + 1, 0, matrixVals[MatrixWidth], 0, matrix->Color(0, 150, 50*prev_layer));
        matrix->show();
      } else {
        matrix->drawLine(0, 0, controlVals[EncoderVal], 0, matrix->Color(0, 150, 0));
        matrix->show();
      }
    }
  }
}

//=====================================EFFECTS=====================================//
//==================================One Dimension==================================//
// The default KOLOR effect
void default_effect(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  // Scale the width and pixel midpoint so they're within range of the number of LEDS (matrixSize)
  int16_t halfWidth = (map((uint16_t)computeOutputValue(controlVals[Param1], controlVals[Param1In]), 0, 255, 1, matrixSize)) >> 1; // Total width halved
  uint16_t pixelMidpoint = map((uint16_t)computeOutputValue(controlVals[Param2], controlVals[Param2In]), 0, 255, 0, matrixSize-1); // Midpoint is 0-indexed so matrixSize-1

  // Clamp start and end indices
  uint16_t startPixel = (pixelMidpoint > halfWidth) ? (pixelMidpoint - halfWidth) : 0;
  uint16_t endPixel = pixelMidpoint + halfWidth;
  if (endPixel >= matrixSize) endPixel = matrixSize - 1;

  // Prepare color and clear buffer
  const CRGB col = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);
  FastLED.clear();

  // Skip controls "density" of the effect
  uint8_t skip = 1 + (controlVals[EncoderVal] % 16);

  // Set the right side leds
  for (uint16_t i = pixelMidpoint; i <= endPixel; i += skip) {
    leds[i] = col;
  }

  // Set the left side leds
  for (int16_t i = pixelMidpoint - skip; i >= (int16_t)startPixel; i -= skip) {
    leds[i] = col;
  }
  
  FastLED.show();
}
/*
// Pulsing LEDs with fade
void pulse(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  static unsigned long last_pulse, last_fade;
  bool should_render = false;

  // Parameters
  bool ext_clock = controlVals[Param2In] > 127; // External clock
  uint16_t pulse_time = controlVals[EncoderVal] << 3; // Time it takes for a pulse to finish in ms. range of 0-2040
  uint16_t width = map((uint16_t)computeOutputValue(controlVals[Param1], controlVals[Param1In]), 0, 255, 1, matrixSize); // Total width for LED block
  uint16_t mid_point = map((uint16_t)controlVals[Param2], 0, 255, 0, matrixSize-1); // Midpoint for LED block
  uint16_t fade_time = pulse_time >> 8; // Time it takes to for a fade to finish
  uint8_t fade_increment = (pulse_time > 1300) ? 2 : (pulse_time > 800) ? 6 : 15; // Ratio to fade by each fade
  
  // Initialise everything on first tick
  if (first_tick) {
    last_pulse = 0;
    last_fade = 0;
    should_render = true;
    FastLED.clear();
  }
  
  // Clamp start pixel and adjust width
  uint8_t start_pixel;
  clamp_led_block(width, mid_point, matrixSize, start_pixel);

  // Prepare color and clear buffer
  const CRGB col = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);

  // Pulse on an external clock OR pulse if the pulse time is reached
  if ((pulse_time <= 40 && ext_clock) || (clock - last_pulse) >= pulse_time) {
    last_pulse = clock;
    fill_solid(&leds[start_pixel], width, col);
    should_render = true;
  }

  should_render = fade_leds(leds, matrixSize, fade_increment, last_fade, clock, fade_time);
  
  // Render if required
  if (should_render) {
    FastLED.show();
  }
}
*/

// A fading trail to moving LEDs
void trails(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {

  // Keep track of the last fade for timing
  static unsigned long last_fade; 

  // Parameters
  uint8_t fade_time = controlVals[EncoderVal] >> 2; // Time it takes to for a fade to finish. Range is 0-63
  uint16_t width = map((uint16_t)computeOutputValue(controlVals[Param1], controlVals[Param1In]), 0, 255, 1, matrixSize); // Total width for LED block
  uint16_t mid_point = map((uint16_t)computeOutputValue(controlVals[Param2], controlVals[Param2In]), 0, 255, 0, matrixSize-1); // Midpoint for LED block
  uint8_t fade_increment = fade_time > 36 ? 4 : fade_time > 15 ? 10 : 20; // Ratio to fade by each fade

  // Initialise everything on first tick 
  // NOTE: Don't need to clear every loop as we fade to black instead which clears the screen
  if (first_tick) {
    last_fade = 0;
    FastLED.clear();
  }

  // Clamp start pixel and adjust width
  uint8_t start_pixel;
  clamp_led_block(width, mid_point, matrixSize, start_pixel);
  
  // Colour the leds within the width starting at the start pixel
  fill_solid(
    &leds[start_pixel], 
    width, 
    color(
      controlVals[RgbSwitch], 
      controlVals[Red], 
      controlVals[Green], 
      controlVals[Blue]
    )
  );

  // Fade every time the fade_time is reached
  if ((clock - last_fade) >= fade_time) {
    last_fade = clock;
    fadeToBlackBy(leds, matrixSize, fade_increment);
  }

  fade_leds(leds, matrixSize, fade_increment, last_fade, clock, fade_time);

  FastLED.show();
}

// Rainbow LEDs with pulse
void rainbow_pulse(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  static unsigned long last_pulse, last_fade;
  bool should_render = false;

  // Parameters
  bool ext_clock = controlVals[Param2In] > 127; // External clock
  uint16_t pulse_time = controlVals[EncoderVal] << 3; // Time it takes for a pulse to finish in ms. range of 0-2040
  uint16_t width = map((uint16_t)computeOutputValue(controlVals[Param1], controlVals[Param1In]), 0, 255, 1, matrixSize); // Total width for LED block
  uint16_t mid_point = map((uint16_t)controlVals[Param2], 0, 255, 0, matrixSize-1); // Midpoint for LED block
  uint16_t fade_time = pulse_time >> 8; // Time it takes to for a fade to finish
  uint8_t fade_increment = (pulse_time > 1300) ? 2 : (pulse_time > 800) ? 6 : 15; // Ratio to fade by each fade
  
  // Initialise everything on first tick
  if (first_tick) {
    last_pulse = 0;
    last_fade = 0;
    should_render = true;
    FastLED.clear();
  }
  
  // Clamp start pixel and adjust width
  uint8_t start_pixel;
  clamp_led_block(width, mid_point, matrixSize, start_pixel);

  // Pulse on an external clock OR pulse if the pulse time is reached
  if ((pulse_time <= 40 && ext_clock) || (clock - last_pulse) >= pulse_time) {
    last_pulse = clock;
    fill_rainbow(&leds[start_pixel], width, controlVals[Red], controlVals[Green]);
    should_render = true;
  }

  should_render = fade_leds(leds, matrixSize, fade_increment, last_fade, clock, fade_time);
  
  // Render if required
  if (should_render) {
    FastLED.show();
  }
}

// Rainbow LEDs with trails
void rainbow_trails(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {

  // Keep track of the last fade for timing
  static unsigned long last_fade;

  // Parameters
  uint8_t fade_time = controlVals[EncoderVal] >> 2; // Time it takes to for a fade to finish. Range is 0-63
  uint16_t width = map((uint16_t)computeOutputValue(controlVals[Param1], controlVals[Param1In]), 0, 255, 1, matrixSize); // Total width for LED block
  uint16_t mid_point = map((uint16_t)computeOutputValue(controlVals[Param2], controlVals[Param2In]), 0, 255, 0, matrixSize-1); // Midpoint for LED block
  uint8_t fade_increment = fade_time > 36 ? 4 : fade_time > 15 ? 10 : 20; // Ratio to fade by each fade

  // Initialise everything on first tick
  if (first_tick) {
    last_fade = 0;
    FastLED.clear();
  }

  // Clamp start pixel and adjust width
  uint8_t start_pixel;
  clamp_led_block(width, mid_point, matrixSize, start_pixel);
  
  // Refresh the LEDS
  fill_rainbow(&leds[start_pixel], width, controlVals[Red], controlVals[Green]);

  fade_leds(leds, matrixSize, fade_increment, last_fade, clock, fade_time);

  FastLED.show();
}

// LEDs chase down strip on trigger
void chase(CRGB* leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  bool should_render = false;
  uint8_t end_index = matrixSize - 1;

  // Keep track of the last chase and fade for timing
  static unsigned long last_chase;
  static unsigned long last_fade;

  // Tells us if we have reached the end two LEDS
  static bool last;

  // Parameters
  uint8_t width = 5;
  uint8_t chase_time = map(controlVals[Param1], 0, 255, 1, 20); // Time it takes for chase to finish
  uint8_t chase_increment = chase_time > 15 ? 1 : chase_time > 6 ? 2 : 4; // How many pixels to move each chase
  uint8_t fade_time = controlVals[EncoderVal] >> 2; // Time it takes to for a fade to finish. Range is 0-63
  uint8_t fade_increment = fade_time > 38 ? 4 : fade_time > 20 ? 10 : 20; // Ratio to fade by each fade
  bool ext_clock = controlVals[Param2In] > 127; // External clock
  uint8_t manual_clock = controlVals[Param2]; // Manual clock
  
  static uint8_t start_index;
  static bool prev_trigger;

  // Initial setup on the first tick
  if (first_tick) {
    last_chase = 0;
    last_fade = 0;
    last = false;
    start_index = 0;
    should_render = true;
    FastLED.clear();
  }

  bool trigger = false;
  bool untrigger = false;
  if (manual_clock == 0) {
    // Use external clock
    trigger = ext_clock && !prev_trigger;
    untrigger = !ext_clock;
  } else {
    // Use manual clock
    trigger = (manual_clock >= 127) && !prev_trigger;
    untrigger = (controlVals[Param2] < 127);
  }

  if (trigger) {
    start_index = 0;
    should_render = true;
    prev_trigger = true;
    last = false;
    last_chase = 0;
    last_fade = 0;
  } else if (untrigger) {
    prev_trigger = false;
  }

  // Shorten width if it exceeds the number of LEDS
  if (start_index + width >= end_index) {
    width = (end_index - start_index);
  }
    
  // Fill the leds with certain colour
  CRGB col = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);
  fill_solid(&leds[start_index], width, col);
  
  // Make sure to render the last led
  if (start_index >= end_index) {
    if (!last) {
        leds[end_index] = col;
        last = true;
        should_render = true;
      } else { 
        should_render = false;
      }

  // And increment position each time the chase_time is reached
  } else {
    if ((clock - last_chase) >= chase_time) {
      last_chase = clock;
      if (start_index + chase_increment <= end_index) {
        start_index += chase_increment;
      } else if (chase_increment != 1 && start_index < end_index) {
        start_index++;
      }
      should_render = true;
    }
  }

  should_render = fade_leds(leds, matrixSize, fade_increment, last_fade, clock, fade_time);

  if (should_render) {
    FastLED.show();
  }
}

// Sparkle effect on trigger
void sparkle(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  bool should_render = false;
  
  // Last fade time
  static unsigned long last_sparkle, last_fade;
  
  // Parameters
  uint8_t sparkle_chance = controlVals[Param1] == 0 ? 1 : controlVals[Param1]; // Chance of sparkle to occur and ensure it isn't 0
  uint8_t fade_increment = map(computeOutputValue(controlVals[Param2], controlVals[Param2In]), 0, 255, 20, 255); // How much to fade by each loop
  uint8_t sparkle_num = map(controlVals[EncoderVal], 0, 255, 1, 50); // Number of sparkles to add each time one occurs
  uint8_t halfRange = controlVals[Param1In] >> 1;
  uint8_t hueMid = controlVals[Red];
  int16_t hueStart = hueMid - halfRange < 0 ? 0 : hueMid - halfRange;
  uint16_t hueEnd = hueMid + halfRange > 255 ? 255 : hueMid + halfRange;
  
  fadeToBlackBy(leds, matrixSize, fade_increment);

  // Higher sparkle chance == greater likelihood a sparkle will occur
  if(random8() < sparkle_chance) {
    for (uint8_t i = 0; i < sparkle_num; i++) {
      leds[random16(matrixSize)] += CHSV(random(hueStart, hueEnd),controlVals[Green], controlVals[Blue]);
    }
  }

  FastLED.show();
}

//==================================Two Dimension==================================//
// Draws a circle
void circles_2d(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  // Only render if required
  bool should_render = false;

  // Keep track of the last decay for timing
  static unsigned long last_decay;
  
  if (first_tick) {
    last_decay = 0;
    should_render = true;
    matrix->clear();
  }

  // Parameters
  uint8_t radius = map(controlVals[EncoderVal], 0, 255, 0, 30);
  uint8_t x_pos = map(computeOutputValue(controlVals[Param1], controlVals[Param1In]), 0, 255, 0, matrixVals[MatrixWidth]);
  uint8_t y_pos = map(computeOutputValue(controlVals[Param2], controlVals[Param2In]), 0, 255, 0, matrixVals[MatrixHeight]);

  // HSV or RGB
  if (controlVals[RgbSwitch]) {
    uint16_t col = matrix->Color(controlVals[Red], controlVals[Green], controlVals[Blue]);
    switch (matrixVals[Shape]) {
      case 0:
        matrix->drawRect(x_pos, y_pos, 2*radius, 2*radius, col);
        break;
      case 1:
        matrix->drawCircle(x_pos, y_pos, radius, col);
        break;
      case 2:
        matrix->drawTriangle(x_pos, y_pos, x_pos-(radius>>1), y_pos-radius, x_pos+(radius>>1), y_pos-radius, col);
        break;
    }
  } else {
    CHSV hsv = CHSV(controlVals[Red], controlVals[Green], controlVals[Blue]);
    CRGB rgb;
    hsv2rgb_rainbow(hsv, rgb);  //convert HSV to RGB
    uint16_t col = matrix->Color(rgb.r, rgb.g, rgb.b);
    switch (matrixVals[Shape]) {
      case 0:
        matrix->drawRect(x_pos, y_pos, 2*radius, 2*radius, col);
        break;
      case 1:
        matrix->drawCircle(x_pos, y_pos, radius, col);
        break;
      case 2:
        matrix->drawTriangle(x_pos, y_pos, x_pos-(radius/2), y_pos-radius, x_pos+(radius/2), y_pos-radius, col);
        break;
    }
  }

  if ((clock - last_decay) >= 3) {
    // approx every 8 ms
    last_decay = clock;
    fadeToBlackBy(leds, matrixSize, 6);
    should_render = true;
  }

  if (should_render) {
    matrix->show();
  }
}

// Prints Repeated ASCII characters
void ascii_2d(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  matrix->fillScreen(0);
  matrix->setTextWrap(false);
  matrix->setTextSize(1);
  
  // Parameters
  uint8_t x_pos = map(computeOutputValue(controlVals[Param1], controlVals[Param1In]), 0, 255, 0, matrixVals[MatrixWidth]);
  uint8_t ascii = map(computeOutputValue(controlVals[Param2], controlVals[Param2In]), 0, 255, 0, 127);
  uint8_t count = controlVals[EncoderVal];
  matrix->setCursor(x_pos,0);

  // HSV or RGB
  if (controlVals[RgbSwitch]) {
    matrix->setTextColor(matrix->Color(controlVals[Red], controlVals[Green], controlVals[Blue]));
  } else {
    CHSV hsv = CHSV(controlVals[Red], controlVals[Green], controlVals[Blue]);
    CRGB rgb;
    hsv2rgb_rainbow(hsv, rgb);  //convert HSV to RGB
    matrix->setTextColor(matrix->Color(rgb.r, rgb.g, rgb.b));
  }

  // Print as many ascii as required
  for (int i = 0; i < count; i++) {
    matrix->print((char) ascii);
  }

  matrix->show();
}
