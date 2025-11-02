# KOLOR
#### A Eurorack utility module for controlling RGB LED Pixel Strips with Eurorack CV signals.

## General Flow
### Start up
1. Setup hardware, MCU pins, led matrix.
2. Read and verify all values from eeprom, otherwise use default values.
   This includes matix sizes and orientation values as well as effect specific values
   and the current mode.

### Loop
1. Check for any serial writes from the Konfig app, and parse accordingly.
2. Refresh all controls (pots, encoder, CV inputs) and check for events from the encoder switch.
3. If in setup mode, handle clocks and long press different to normal mode.
4. Fetch the saved value from the current encoder tracker for the current mode and pass all 
   values to the current effect function. effect functions are part of a function pointer array
   and are indexed into in order of effect.

## Architecture
Each effect is defined in `effects.h`, and `effects.cpp`. These are just functions requiring
the same arguments, which then allows us to declare a global function pointer array in 
`kolor.ino`. Each effect is insterted into the fp array in the same order as the `Mode` enum.

There are a few helper functions in the effects files including `color()`, `clamp_led_block()`,
`computeOutputValue()` and `fade_leds()`. These pretty much do exactly what they say with 
creating RGB or HSV objects, clamping pixels within the bounds of the panel, computing output
values for non-linked pots and CV inputs, and fading leds by a certain amount. These just clean
up the effect function and make the system more modular.

## Switches and other IO
All I/O is defined in `elkIO.h` and `elkIO.cpp` as classes. Essentially, 
`AnalogIn` handles potentiometer controls, `Switch` and `EventSwitch` handle the encoder
push switch and `EncoderTracker` handles turning the encoder.

## Potential Improvements
While writing this, I noticed a few improvements that could be made:
1. Making effects classes instead of functions (See the MidiKameleon implementation).
2. Passing states and values in a struct instead of multiple values into effect
   process functions. This would look cleaner, but behave the same.
3. Code naming and styling consistency could be improved. For example, `clamp_led_block()` and 
   `computeOutputValue()` both are just helper functions, but have a clear styling difference
