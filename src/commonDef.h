#pragma once

#define MAX_INT16F (float)std::numeric_limits<int16_t>::max()

// INMP441 Microphone
#define I2S_PORT I2S_NUM_0
#define I2S_WS 26
#define I2S_SD 25
#define I2S_BCK 27
#define SAMPLE_RATE 30000
#define BufferLen 512

// PCM5102 Digital-to-Analog Converter (DAC)
#define I2S_DOUT 22

// Equalizer
#define NB_BANDS 6

#define NB_STATES 3
enum State {
    VISUAL = 0,
    AUDIO = 1,
    MODIF = 2
};
