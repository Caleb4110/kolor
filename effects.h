#ifndef EFFECTS_H
#define EFFECTS_H

// LED libraries for 1d and 2d drawing
#include <FastLED.h>
#include <Adafruit_GFX.h>
#include <FastLED_NeoMatrix.h>

// Other
#include "common.h"

//===================================UTILITIES=====================================//
// Color function for FastLED library
inline CRGB color(bool use_rgb, uint8_t r_or_h, uint8_t g_or_s, uint8_t b_or_v);


//====================================SETTINGS=====================================//

/*
| Parameter       | Usage                             |
| --------------  | --------------------------------- |
| HSV/RGB         | Switch between width/height       |
| Red/Hue         | 1st LED vertical position toggle  |
| Green/Sat       | 1st LED horizontal position toggle|
| Blue/Val        | Panel orientation toggle          |
| Param 1         | Panel layout toggle               |
| Param 2         | Unbound                           |
| Select          | Number of LEDS                    |
*/
void setup_matrix(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]);


//=====================================EFFECTS=====================================//
//==================================One Dimension==================================//
/*
| Parameter       | Usage                             |
| --------------  | --------------------------------- |
| HSV/RGB         | Switch between HSV & RGB lighting |
| Red/Hue         | Red/hue value                     |
| Green/Sat       | Green/sat value                   |
| Blue/Val        | Blue/val value                    |
| Param 1         | Width                             |
| Param 2         | Position                          |
| Select          | LED spacing                       |
*/
void default_effect(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]);

/*
| Parameter       | Usage                             |
| --------------  | --------------------------------- |
| HSV/RGB         | Switch between HSV & RGB lighting |
| Red/Hue         | Red/hue value                     |
| Green/Sat       | Green/sat value                   |
| Blue/Val        | Blue/val value                    |
| Param 1         | Width                             |
| Param 2         | Position                          |
| Select          | Pulse time                        |
| Ext. Clock      | Pulse time                        |

NOTE: To use the external clock, turn the `Select` knob left until LEDs stop flashing, KOLOR will
then take the external clock as input for the pulse time.
*/
//void pulse(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]);

/*
| Parameter       | Usage                             |
| --------------  | --------------------------------- |
| HSV/RGB         | Switch between HSV & RGB lighting |
| Red/Hue         | Red/hue value                     |
| Green/Sat       | Green/sat value                   |
| Blue/Val        | Blue/val value                    |
| Param 1         | Width                             |
| Param 2         | Position                          |
| Select          | Trail time                        |
*/
void trails(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]);

/*
| Parameter       | Usage                             |
| --------------  | --------------------------------- |
| HSV/RGB         | Unbound                           |
| Red/Hue         | Hue shift                         |
| Green/Sat       | Colour repeats                    |
| Blue/Val        | Unbound                           |
| Param 1         | Width                             |
| Param 2         | Position                          |
| Select          | Pulse time                        |
| Ext. Clock      | Pulse time                        |

NOTE: To use the external clock, turn the `Select` knob left until LEDs stop flashing, KOLOR will
then take the external clock as input for the pulse time.
*/
void rainbow_pulse(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]);

/*
| Parameter       | Usage                             |
|---------------  | --------------------------------- |
| HSV/RGB         | Unbound                           |
| Red/Hue         | Hue shift                         |
| Green/Sat       | Colour repeats                    |
| Blue/Val        | Unbound                           |
| Param 1         | Width                             |
| Param 2         | Position                          |
| Select          | Trail Time                        |
*/
void rainbow_trails(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]);

/*
| Parameter       | Usage                             |
|---------------  | --------------------------------- |
| HSV/RGB         | Switch between HSV & RGB lighting |
| Red/Hue         | Red/hue value                     |
| Green/Sat       | Green/sat value                   |
| Blue/Val        | Blue/val value                    |
| Param 1         | Chase speed                       |
| Param 2         | Manual chase trigger over 50%     |
| Select          | Fade speed                        |
| Ext. Clock      | Chase trigger                     |

NOTE: To use the external clock, turn the `Param 2` knob hard left. KOLOR will
then take the external clock as input for triggering a chase.
*/
void chase(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]);

/*
| Parameter       | Usage                             |
| --------------  | --------------------------------- |
| HSV/RGB         | Unbound                           |
| Red/Hue         | Hue randomisation range           |
| Green/Sat       | Saturation                        |
| Blue/Val        | Value                             |
| Param 1         | Sparkle chance                    |
| Param 2         | Fade speed                        |
| Select          | Number of pixels per sparkle      |
*/
void sparkle(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]);


//==================================Two Dimension==================================//

/*
| Parameter       | Usage                             |
| --------------  | --------------------------------- |
| HSV/RGB         | Switch between HSV & RGB lighting |
| Red/Hue         | Red/hue value                     |
| Green/Sat       | Green/sat value                   |
| Blue/Val        | Blue/val value                    |
| Param 1         | Position 1                        |
| Param 2         | Position 2                        |
| Select          | Circle size                       |
*/
void circles_2d(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]);

/*
| Parameter       | Usage                             |
| --------------  | --------------------------------- |
| HSV/RGB         | Switch between HSV & RGB lighting |
| Red/Hue         | Red/hue value                     |
| Green/Sat       | Green/sat value                   |
| Blue/Val        | Blue/val value                    |
| Param 1         | x position                        |
| Param 2         | ASCII character                   |
| Select          | Character repeats                 |
*/
void ascii_2d(CRGB *leds, FastLED_NeoMatrix *matrix, uint16_t matrixSize, unsigned long clock, uint8_t first_tick, uint8_t controlVals[], uint8_t matrixVals[]);

#endif // EFFECTS_H
