#ifndef LAYER_CONFIG_H
#define	LAYER_CONFIG_H

// Notes:
// - In order to achieve the desired refresh interval, make sure the TLC5940 uses a sufficient GSCLK PWM frequency
// - When the interlaced config is not used, you'll need to reduce the refresh interval to around 1000us
// - When the interlaced config is used, the minimum refresh interval is around 1200us

#define LAYER_REFRESH_INTERVAL      1100    // Refresh interval between layers in us, a bit lower than 1200us to make it smoother
#define LAYER_INTERLACED                    // Uncomment to use incremental scanning of the rows

#endif	/* LAYER_CONFIG_H */