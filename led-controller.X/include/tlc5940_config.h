#ifndef TLC5940_CONFIG_H
#define TLC5940_CONFIG_H

#define TLC5940_NUM_OF_DEVICES      3       // Number of TLC5940's daisy chained
#define TLC5940_GSCLK_PERIOD        1100    // Time in us of one GSCLK period
#define TLC5940_DOT_CORRECTION      28      // 6 bit value

// Notes:
// - When the GSCLK period is a small value we might run into problems where 
//   we can't update the TLC5940s with new data before the GSCLK period finishes.
//   This will cause a system abort, see `pwm_period_callback` for more details.
//   To see if the GSCLK period is too low, run the software in debug and
//   see if the assertion in the `pwm_period_callback` is triggered.

#endif /* TLC5940_CONFIG_H */