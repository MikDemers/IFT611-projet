#pragma once

#include <dsps_fft2r.h>
#include <utils.h>
#include <userTask.h>
#include <cmath>

bool dsps_initialized = false;
float fft_buffer[2 * BufferLen];
float bands_buffer[NB_BANDS];
std::pair<double, double> bandsRange[NB_BANDS]{
    {20, 100},      // 50 Hz (Bass)
    {100, 250},     // 150 Hz (Low-mids)
    {250, 650},     // 400 Hz (Mids)
    {650, 1800},    // 1000 Hz (High-mids)
    {1800, 5000},   // 3000 Hz (Presence)
    {5000, 15000}   // 8000 Hz (Brilliance)
};

void displaySetup();
void displayLoop();

void displayTask(void*) {
    while (true) {
        displayLoop();
    }
}

void displaySetup() {  
    dsps_fft2r_init_fc32(NULL, BufferLen);
}

void displayLoop() {
    //static TickType_t lastWakeTime = xTaskGetTickCount();

    while (outProcessingBufferMutex.test_and_set(std::memory_order_acquire)) { ; }

    int samplesRead = nbSamplesRead;

    // Le standard pour ESP-DSP est [real_1, imag_1, real_2, imag_2, ...]
    // On ne fait que la partie réelle avant de libérer la ressource.
    for (int i = 0; i != samplesRead; ++i) {
        fft_buffer[i * 2] = ((float)outProcessingBuffer[i] / MAX_INT16F);
    }
    
    outProcessingBufferMutex.clear(std::memory_order_release);

    // Terminer la partie imaginaire.
    for (int i = 0; i != samplesRead; ++i) {
        fft_buffer[i * 2 + 1] = 0.0f;
    }
    
    // La FFT
    dsps_fft2r_fc32(fft_buffer, BufferLen);
    // "Inversion de bit". Je sais pas vraiment c'est quoi mais c'est recommandé.
    dsps_bit_rev_fc32(fft_buffer, BufferLen);
    
    bandMagnitudes(fft_buffer, samplesRead, bands_buffer, bandsRange);

    Serial.println(bands_buffer[0]);


    for (int i = 0; i != NB_BANDS; ++i) {
        mainScreen->bandLevels[i] = (short) bands_buffer[i];
    }

    //vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(50));
}
