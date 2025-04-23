#pragma once

#include <driver/i2s.h>
#include <AudioModule.h>
#include <atomic>
#include <commonDef.h>
#include <chrono>


// -=- Variables -=-

extern short state;

// Audio buffers (initialized in order of use)
int32_t inBuffer[BufferLen];
int nbSamplesRead;
float inProcessingBuffer[BufferLen];
float outProcessingBuffer[BufferLen];
int16_t outStereoBuffer[BufferLen * 2];

// outProcessingBuffer et nbSamplesRead seront accédés simultanément à partir du displayTask.
// On lock les deux ensemble pour qu'ils soient cohérents.
// Spin lock pour simplicité et sans l'overhead de mutex.
// src implémentation: https://stackoverflow.com/questions/26583433/c11-implementation-of-spinlock-using-header-atomic
std::atomic_flag outProcessingBufferMutex = ATOMIC_FLAG_INIT;

AudioModule audioMod;


// -=- Declarations -=-

void audioSetup();
void audioLoop();

void i2s_install();
void i2s_setpin();

void testDAC();


// -=- Definitions -=-

void audioTask(void*) {
    while (true) {
        audioLoop();
    }
}

void audioSetup() {
    i2s_install();
    i2s_setpin();
    i2s_start(I2S_PORT);

    testDAC();
    delay(1000);
    
    Serial.println("Audio setup complete.");
}

void audioLoop() {
    auto t0 = std::chrono::high_resolution_clock::now();
    size_t nbBytesRead = 0;
    
    // Take input from I2C in buffer
    esp_err_t result = i2s_read(I2S_PORT, inBuffer, BufferLen * sizeof(int32_t), &nbBytesRead, 10);
    
    if (result == ESP_OK && nbBytesRead > 0) {      
        while (outProcessingBufferMutex.test_and_set(std::memory_order_acquire)) { ; }
        // Convert from 32-bit to 16-bit and then to float.
        nbSamplesRead = nbBytesRead / sizeof(int32_t);
        for (int i = 0; i < nbSamplesRead; i++) {
            inProcessingBuffer[i] = (float)(inBuffer[i] >> 16);
        }

        // The real meat of this function
        auto t1 = std::chrono::high_resolution_clock::now();
        audioMod.apply(inProcessingBuffer, outProcessingBuffer, (size_t) nbSamplesRead);
        auto t2 = std::chrono::high_resolution_clock::now();
        auto t_audioMod = t2 - t1;
        
        outProcessingBufferMutex.clear(std::memory_order_release);
        
        if (state == State::AUDIO) {
            // Convert to stereo and clip
            for (int i = 0; i < nbSamplesRead; i++) {
                float sample = outProcessingBuffer[i];
                if (sample > MAX_INT16F) sample = MAX_INT16F;
                if (sample < -MAX_INT16F) sample = -MAX_INT16F;
                
                outStereoBuffer[i * 2] = (int16_t)sample;
                outStereoBuffer[i * 2 + 1] = (int16_t)sample;
            }
        } else {
            if (state != State::AUDIO) {
                std::fill(outStereoBuffer, outStereoBuffer + BufferLen * 2, 0);
            }
        }
        
        // Output to I2C out buffer
        size_t nb_bytes_written = 0;
        result = i2s_write(I2S_PORT, outStereoBuffer, nbSamplesRead * 2 * sizeof(int16_t), &nb_bytes_written, 10);
        
        auto t3 = std::chrono::high_resolution_clock::now();
        auto t_total = t3 - t0;
        
        /* Serial.print("audioMod.apply(): ");
        Serial.print(std::chrono::duration_cast<std::chrono::microseconds>(t_audioMod).count());
        Serial.print(" | audioLoop: ");
        Serial.print(std::chrono::duration_cast<std::chrono::microseconds>(t_total).count());
        Serial.print("\n\n"); */
    }
}

void i2s_install() {
    const i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = i2s_bits_per_sample_t(32),
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = BufferLen,
        .use_apll = true
    };

    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("Failed to install I2S driver: %d\n", err);
    }
}

void i2s_setpin() {
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_SD
    };

    i2s_set_pin(I2S_PORT, &pin_config);
}

void testDAC() {
    int16_t stereoBuffer[BufferLen * 2];

    // Generate a sine wave.
    for (int i = 0; i < BufferLen; i++) {
        int16_t sample = std::sin(i * 0.05) * 1000;
        stereoBuffer[i * 2] = sample;
        stereoBuffer[i * 2 + 1] = sample;
    }

    size_t bytesOut = 0;
    i2s_write(I2S_PORT, stereoBuffer, BufferLen * 2 * sizeof(int16_t), &bytesOut, 10);
}
