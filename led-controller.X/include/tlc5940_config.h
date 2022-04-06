#ifndef TLC5940_CONFIG_H
#define TLC5940_CONFIG_H

#define TLC5940_NUM_OF_DEVICES          3       // Number of TLC5940's daisy chained
#define TLC5940_DOT_CORRECTION          28      // 6 bit value, must be <= 28 (see tlc5940.cpp for the rationale)

// Time in us of one GSCLK period
#ifdef __DEBUG
#define TLC5940_GSCLK_PERIOD            1500
#define TLC5940_DEFFERED_BLANK_DELAY    100
#else
#define TLC5940_GSCLK_PERIOD            975     // Specifies the length, in microseconds, of a single GSCLK period
#define TLC5940_DEFFERED_BLANK_DELAY    65      // Specifies the delay, in microseconds, of how long the blank pin should be held high after a new GSCLK period has started
#endif

// Notes:
// - When the GSCLK period is a small value we might run into problems where 
//   we can't update the TLC5940s with new data before the GSCLK period finishes.
//   This will cause a system abort, see `pwm_period_callback` for more details.
//   To see if the GSCLK period is too low, run the software in debug and
//   see if the assertion in the `pwm_period_callback` is triggered.
// - Make the deffered blank delay too small and we will enable the outputs of
//   the TLC5940s too quickly whilst the FET from the previous layer is not yet
//   fully closed, which in turn will cause a ghosting effect. This is actually
//   a design failure, as turning off the FETs is done via a pull-up resistor
//   instead of using a proper push-pull FET driver. The deferred blank delay
//   is part of the GSCLK period. So making this value too large and we will
//   lose PWM resolution and overall LED brightness (since the LEDs are during
//   deferred blanking).

#endif /* TLC5940_CONFIG_H */
