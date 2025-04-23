#pragma once

// General
#define MAX_INT16F (float)std::numeric_limits<int16_t>::max()
#define NB_BANDS 6
#define NB_STATES 3
short state;
enum State {
    VISUAL = 0,
    AUDIO = 1,
    MODIF = 2
};

// Rotary Encoder
#define SDA 33
#define SCL 32
#define PUSH_BUTTON 4
#define POTENTIO_RIGHT 2
#define POTENTIO_LEFT 15

// INMP441 Microphone
#define I2S_PORT I2S_NUM_0
#define I2S_WS 26
#define I2S_SD 25
#define I2S_BCK 27
#define SAMPLE_RATE 30000
#define BufferLen 256

// PCM5102 Digital-to-Analog Converter (DAC)
#define I2S_DOUT 22
