#include "FastLED.h"
#include "effects.h"

//===================================UTILITIES=====================================//
// Color function for FastLED library
CRGB color(bool use_rgb, uint8_t r_or_h, uint8_t g_or_s, uint8_t b_or_v) {
  if (use_rgb) {
    return CRGB(r_or_h, g_or_s, b_or_v);
  } else {
    return CHSV(r_or_h, g_or_s, b_or_v);
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


//====================================SETTINGS=====================================//
// Setup number of pixels for strip 
void setup_matrix(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  FastLED.clear();

  static unsigned long last_change;
  static uint8_t lastVals[4];
  static CRGB currentColour;

  // Indexing array to change led colour efficiently
  CRGB colours[4][2] = {
    { 
      CRGB(50, 0, 0),
      CRGB(50, 50, 0),
    },
    { 
      CRGB(0, 50, 0),
      CRGB(0, 50, 50),
    },
    { 
      CRGB(0, 0, 50),
      CRGB(50, 0, 50),
    },
    { 
      CRGB(50, 10, 20),
      CRGB(50, 10, 0),
    },
  };
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
  for (uint8_t i = 1; i < 5; i++) {
    const uint8_t controlVal = controlVals[i] < (255 >> 1) ? 0 : 1;
    if (lastVals[i-1] != controlVal) {
      lastVals[i-1] = controlVal;

      currentColour = colours[i-1][controlVal];
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

  int16_t startPixel = pixelMidpoint - halfWidth;
  int16_t endPixel = pixelMidpoint + halfWidth;  

  // we've wrapped around, let's wrap back
  if (startPixel < 0) {
    startPixel = 0;
  }

  // clamp to end
  if (endPixel >= matrixSize) {
    endPixel = matrixSize - 1;
  }

  // actually render
  FastLED.clear();

  // This determines how many leds are skipped between lit leds
  uint8_t skip = 1 + (controlVals[EncoderVal] % 16);

  // Set the right side leds
  for (int16_t i = pixelMidpoint; i <= endPixel; i += skip) {
    leds[i] = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);
  }

  // Set the left side leds
  for (int16_t i = pixelMidpoint - skip; i >= startPixel; i -= skip) {
    leds[i] = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);
  }
  
  FastLED.show();
}

// Pulsing LEDs with fade
void pulse(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  // Allows us to only render when a change has occurred
  bool should_render = false;
  
  // Keep track of the last pulse and fade for timing
  static unsigned long last_pulse;
  static unsigned long last_fade;

  // Parameters
  uint16_t ext_clock = map(controlVals[Param2In], 0, 255, 0, 1); // External clock
  uint16_t pulse_time = map(controlVals[EncoderVal], 0, 255, 0, 2048); // Time it takes for a pulse to finish
  uint16_t width = map((uint16_t)computeOutputValue(controlVals[Param1], controlVals[Param1In]), 0, 255, 1, matrixSize); // Total width for LED block
  int16_t mid_point = map((uint16_t)controlVals[Param2], 0, 255, 0, matrixSize-1); // Midpoint for LED block
  int16_t start = mid_point - (width >> 1); // Start pixel for LED block
  uint16_t fade_time = pulse_time >> 8; // Time it takes to for a fade to finish
  uint8_t fade_increment = (pulse_time > 1300) ? 2 : (pulse_time > 800) ? 6 : 15; // Ratio to fade by each fade
  
  // Initialise everything on first tick
  if (first_tick) {
    last_pulse = 0;
    last_fade = 0;
    should_render = true;
    FastLED.clear();
  }

  // Check if start is out of range
  if (start < 0) {
    width += start;
    start = 0;
  }

  // Shorten width if it exceeds the number of LEDS
  if (start + width >= matrixSize) {
    width = (matrixSize - start);
  }

  // Either use pulse_time OR external clock
  if (pulse_time <= 40) {
    if (ext_clock) {
      CRGB c = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);
      fill_solid(&leds[start], width, c);
      should_render = true;
    }
  } else if ((clock - last_pulse) >= pulse_time) {
    last_pulse = clock;
    CRGB c = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);
    fill_solid(&leds[start], width, c);
    should_render = true;
  }

  // Fade every time the fade_time is reached
  if ((clock - last_fade) >= fade_time) {
    last_fade = clock;
    fadeToBlackBy(leds, matrixSize, fade_increment);
    should_render = true;
  }
  
  // Render if required
  if (should_render) {
    FastLED.show();
  }
}

// A fading trail to moving LEDs
void trails(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {

  // Keep track of the last fade for timing
  static unsigned long last_fade;

  // Parameters
  uint8_t fade_time = map(controlVals[EncoderVal], 0, 255, 0, 50); // Time it takes to for a fade to finish
  uint16_t width = map((uint16_t)computeOutputValue(controlVals[Param1], controlVals[Param1In]), 0, 255, 1, matrixSize); // Total width for LED block
  int16_t mid_point = map((uint16_t)computeOutputValue(controlVals[Param2], controlVals[Param2In]), 0, 255, 0, matrixSize-1); // Midpoint for LED block
  int16_t start = mid_point - (width >> 1); // Start pixel for LED block
  uint8_t fade_increment = fade_time > 36 ? 4 : fade_time > 15 ? 10 : 20; // Ratio to fade by each fade

  // Initialise everything on first tick 
  // NOTE: Don't need to clear every loop as we fade to black instead which clears the screen
  if (first_tick) {
    last_fade = 0;
    FastLED.clear();
  }

  // Check if start is out of range
  if (start < 0) {
    width += start;
    start = 0;
  }

  // Shorten width if it exceeds the number of LEDS
  if (start + width >= matrixSize) {
    width = (matrixSize - start);
  }
  
  // Colour the leds within the width starting at the start point
  CRGB c = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);
  fill_solid(&leds[start], width, c);

  // Fade every time the fade_time is reached
  if ((clock - last_fade) >= fade_time) {
    last_fade = clock;
    fadeToBlackBy(leds, matrixSize, fade_increment);
  }

  FastLED.show();
}

// Rainbow LEDs
void rainbow(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {

  // Parameters
  uint16_t width = map((uint16_t)computeOutputValue(controlVals[Param1], controlVals[Param1In]), 0, 255, 1, matrixSize); // Total width for LED block
  int16_t mid_point = map((uint16_t)computeOutputValue(controlVals[Param2], controlVals[Param2In]), 0, 255, 0, matrixSize-1); // Midpoint for LED block
  int16_t start = mid_point - (width >> 1); // Start pixel for LED block

  // Check if start is out of range
  if (start < 0) {
    width += start;
    start = 0;
  }

  // Shorten width if it exceeds the number of LEDS
  if (start + width >= matrixSize) {
    width = (matrixSize - start);
  }
  
  // Refresh the LEDS
  FastLED.clear();
  fill_rainbow(&leds[start], width, controlVals[Red], controlVals[Green]);
  FastLED.show();
}

// Rainbow LEDs with pulse
void rainbow_pulse(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  // Allows us to only render when a change has occurred
  bool should_render = false;
  
  // Keep track of the last pulse and fade for timing
  static unsigned long last_pulse;
  static unsigned long last_fade;

  // Parameters
  uint16_t ext_clock = map(controlVals[Param2In], 0, 255, 0, 1); // External clock
  uint16_t pulse_time = map(controlVals[EncoderVal], 0, 255, 0, 2048); // Time it takes for a pulse to finish
  uint16_t width = map((uint16_t)computeOutputValue(controlVals[Param1], controlVals[Param1In]), 0, 255, 1, matrixSize); // Total width for LED block
  int16_t mid_point = map((uint16_t)controlVals[Param2], 0, 255, 0, matrixSize-1); // Midpoint for LED block
  int16_t start = mid_point - (width >> 1); // Start pixel for LED block
  uint16_t fade_time = pulse_time >> 8; // Time it takes to for a fade to finish
  uint8_t fade_increment = (pulse_time > 1300) ? 2 : (pulse_time > 800) ? 6 : 15; // Ratio to fade by each fade
  
  // Initialise everything on first tick
  if (first_tick) {
    last_pulse = 0;
    last_fade = 0;
    should_render = true;
    FastLED.clear();
  }

  // Check if start is out of range
  if (start < 0) {
    width += start;
    start = 0;
  }

  // Shorten width if it exceeds the number of LEDS
  if (start + width >= matrixSize) {
    width = (matrixSize - start);
  }

  // Either use pulse_time OR external clock
  if (pulse_time <= 40) {
    if (ext_clock) {
      fill_rainbow(&leds[start], width, controlVals[Red], controlVals[Green]);
      should_render = true;
    }
  } else if ((clock - last_pulse) >= pulse_time) {
    last_pulse = clock;
    fill_rainbow(&leds[start], width, controlVals[Red], controlVals[Green]);
    should_render = true;
  }

  // Fade every time the fade_time is reached
  if ((clock - last_fade) >= fade_time) {
    last_fade = clock;
    fadeToBlackBy(leds, matrixSize, fade_increment);
    should_render = true;
  }
  
  if (should_render) {
    FastLED.show();
  }
}

// Rainbow LEDs with trails
void rainbow_trails(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {

  // Keep track of the last fade for timing
  static unsigned long last_fade;

  // Parameters
  uint8_t fade_time = map(controlVals[EncoderVal], 0, 255, 0, 50); // Time it takes to for a fade to finish
  uint16_t width = map((uint16_t)computeOutputValue(controlVals[Param1], controlVals[Param1In]), 0, 255, 1, matrixSize); // Total width for LED block
  int16_t mid_point = map((uint16_t)computeOutputValue(controlVals[Param2], controlVals[Param2In]), 0, 255, 0, matrixSize-1); // Midpoint for LED block
  int16_t start = mid_point - (width >> 1); // Start pixel for LED block
  uint8_t fade_increment = fade_time > 36 ? 4 : fade_time > 15 ? 10 : 20; // Ratio to fade by each fade

  // Initialise everything on first tick
  if (first_tick) {
    last_fade = 0;
    FastLED.clear();
  }

  // Check if start is out of range
  if (start < 0) {
    width += start;
    start = 0;
  }
  
  // Shorten width if it exceeds the number of LEDS
  if (start + width >= matrixSize) {
    width = (matrixSize - start);
  }
  
  // Refresh the LEDS
  fill_rainbow(&leds[start], width, controlVals[Red], controlVals[Green]);

  // Fade every time the fade_time is reached
  if ((clock - last_fade) >= fade_time) {
    last_fade = clock;
    fadeToBlackBy(leds, matrixSize, fade_increment);
  }

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
  uint8_t chase_time = map(controlVals[Param1], 0, 255, 0, 20); // Time it takes for chase to finish
  uint8_t chase_increment = chase_time > 15 ? 1 : chase_time > 6 ? 2 : 4; // How many pixels to move each chase
  uint8_t fade_time = map(controlVals[EncoderVal], 0, 255, 0, 50); // Time it takes to for a fade to finish
  uint8_t fade_increment = fade_time > 36 ? 4 : fade_time > 15 ? 10 : 20; // Ratio to fade by each fade
  uint8_t ext_clock = map(controlVals[Param2In], 0, 255, 0, 1); // Optional external clock
  uint8_t manual_clock = controlVals[Param2]; // Manual clock
  
  static uint8_t start;
  static bool prev_trigger;

  // Initial setup on the first tick
  if (first_tick) {
    last_chase = 0;
    last_fade = 0;
    last = false;
    start = 0;
    should_render = true;
    FastLED.clear();
  }

  // Decide on what clock to use
  if (manual_clock == 0) {
    if (ext_clock && !prev_trigger) {
      start = 0;
      should_render = true;
      prev_trigger = true;
      last = false;
      last_chase = 0;
      last_fade = 0;
    } else if (!ext_clock) {
      prev_trigger = false;
    }
  } else {
    if (manual_clock >= 127 && !prev_trigger) {
      start = 0;
      should_render = true;
      prev_trigger = true;
      last = false;
      last_chase = 0;
      last_fade = 0;
    } else if (controlVals[Param2] < 127) {
      prev_trigger = false;
    }
  }

  // Shorten width if it exceeds the number of LEDS
  if (start + width >= end_index) {
    width = (end_index - start);
  }
    
  // Fill the leds with certain colour
  CRGB c = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);
  fill_solid(&leds[start], width, c);
  
  // Make sure to render the last led
  if (start >= end_index) {
    if (!last) {
        leds[end_index] = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);
        last = true;
        should_render = true;
      } else { 
        should_render = false;
      }

  // And increment position each time the chase_time is reached
  } else {
    if ((clock - last_chase) >= chase_time) {
      last_chase = clock;
      if (start + chase_increment <= end_index) {
        start += chase_increment;
      } else if (chase_increment != 1 && start < end_index) {
        start++;
      }
      should_render = true;
    }
  }

  // Fade every time the fade_time is reached
  if ((clock - last_fade) >= fade_time) {
    last_fade = clock;
    fadeToBlackBy(leds, matrixSize, fade_increment);
    should_render = true;
  }

  if (should_render) {
    FastLED.show();
  }
}

// Double ended chase on trigger
void bounce(CRGB* leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  bool should_render = false;
  uint8_t end_index = matrixSize - 1;
  
  // Keep track of the last bounce and fade for timing
  static unsigned long last_bounce;
  static unsigned long last_fade;

  // Lets us know if we are up to the end two LEDS
  static bool last;

  // Parameters
  uint8_t width = 10;
  uint8_t bounce_time = map(controlVals[Param1], 0, 255, 0, 20); // Time it takes for chase to finish
  uint8_t bounce_increment = bounce_time > 15 ? 1 : bounce_time > 6 ? 2 : 4; // How many pixels to move each chase
  uint8_t fade_time = map(controlVals[EncoderVal], 0, 255, 0, 50); // Time it takes to for a fade to finish
  uint8_t fade_increment = fade_time > 36 ? 4 : fade_time > 15 ? 10 : 20; // Ratio to fade by each fade
  uint8_t ext_clock = map(controlVals[Param2In], 0, 255, 0, 1); // Optional external clock
  uint8_t manual_clock = controlVals[Param2]; // Manual clock

  // Keep track of left and right positions and if it's been triggered already
  static uint8_t left;
  static uint8_t right;
  static bool prev_trigger;

  // Initial setup on the first tick
  if (first_tick) {
    last_bounce = 0;
    last_fade = 0;
    left = 0;
    right = end_index;
    should_render = true;
    last = false;
    FastLED.clear();
  }

  // Decide on what clock to use
  if (manual_clock == 0) {
    if (ext_clock && !prev_trigger) {
      left = 0;
      right = end_index;
      should_render = true;
      prev_trigger = true;
      last = false;
      last_bounce = 0;
      last_fade = 0;
    } else if (!ext_clock) {
      prev_trigger = false;
    }
  } else {
    if (manual_clock >= 127 && !prev_trigger) {
      left = 0;
      right = end_index;
      should_render = true;
      prev_trigger = true;
      last = false;
      last_bounce = 0;
      last_fade = 0;
    } else if (controlVals[Param2] < 127) {
      prev_trigger = false;
    }
  }

  // Render LEDS at width if time has been reached
  if ((clock - last_bounce) >= bounce_time) {
    last_bounce = clock;

    // Make sure to render the last two leds
    if (left >= end_index && right <= 0) {
      if (!last) {
        leds[0] = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);
        leds[end_index] = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);
        last = true;
        should_render = true;
      } else { 
        should_render = false;
      }
      
    // Otherwise render both left and right blocks 
    } else {
      uint8_t w = left + width >= end_index ? (end_index - left) : width;
      for (int i = 0; i < w; i++) {
        leds[left + i] = color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]);
      }

      w = right + width > end_index ? (end_index - right + 1) : width;
      for (int i = 0; i < w; i++) {
        CRGB c = leds[right + i];
        leds[right + i] = blend(c, color(controlVals[RgbSwitch], controlVals[Red], controlVals[Green], controlVals[Blue]), 200);
      }
      should_render = true;
    }

    // Then increment them
    if (left + bounce_increment <= end_index) {
      left += bounce_increment;
    } else if (bounce_increment != 1 && left < end_index) {
      left++;
    }
    if (right - bounce_increment >= 0) {
      right -= bounce_increment;
    } else if (bounce_increment != 1 && right > 0) {
      right--;
    }
  }

  // Fade every time the fade_time is reached
  if ((clock - last_fade) >= fade_time) {
    last_fade = clock;
    fadeToBlackBy(leds, matrixSize, fade_increment);
    should_render = true;
  }

  if (should_render) {
    FastLED.show();
  }
}

// Sparkle effect on trigger
void sparkle(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]) {
  bool should_render = false;
  
  // Last fade time
  static unsigned long last_sparkle;
  static unsigned long last_fade;
  
  // Parameters
  uint8_t sparkle_chance = map(controlVals[Param1], 0, 255, 1, 255); // Chance of sparkle to occur
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
    matrix->drawCircle(x_pos, y_pos, radius, matrix->Color(controlVals[Red], controlVals[Green], controlVals[Blue]));
  } else {
    CHSV hsv = CHSV(controlVals[Red], controlVals[Green], controlVals[Blue]);
    CRGB rgb;
    hsv2rgb_rainbow(hsv, rgb);  //convert HSV to RGB
    matrix->drawCircle(x_pos, y_pos, radius, matrix->Color(rgb.r, rgb.g, rgb.b));
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
