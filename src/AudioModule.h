#pragma once

#include <Arduino.h>
#include <atomic>
#include <commonDef.h>


class AudioModule {
private:
    std::atomic<float> bandGains[NB_BANDS];

    // Biquad filtering
    // ----------------
    using BiquadCoeffs = float[5];
    struct BiquadFilter {
        float b0, b1, b2;
        float a1, a2;
        float x1, x2;
        float y1, y2;

        void initFilter() {

        }
    };

    // Biquad coefficients for each filter. Can be changed to modify which bands are isolated.
    BiquadCoeffs coeffs[NB_BANDS] = {
        // b0, b1, b2, a1, a2. (Somewhere online I saw coeffs as a0, a1, a2, b0, b1... whatever)
        {0.002611, 0.0, -0.002611, -1.994668, 0.994778},  // 50 Hz
        {0.007792, 0.0, -0.007792, -1.983438, 0.984417},  // 150 Hz
        {0.020491, 0.0, -0.020491, -1.952148, 0.959018},  // 400 Hz
        {0.049410, 0.0, -0.049410, -1.859635, 0.901181},  // 1000 Hz
        {0.128120, 0.0, -0.128120, -1.410732, 0.743761},  // 3000 Hz
        {0.199123, 0.0, -0.199123, 0.167429, 0.601755}    // 8000 Hz
    };
    BiquadFilter filters[NB_BANDS];

    void initBiquad(BiquadFilter& filter, BiquadCoeffs& coeffs) {
        filter.b0 = coeffs[0];
        filter.b1 = coeffs[1];
        filter.b2 = coeffs[2];
        filter.a1 = coeffs[3];
        filter.a2 = coeffs[4];
        filter.x1 = filter.x2 = 0.0f;
        filter.y1 = filter.y2 = 0.0f;
    }

    // The isolation of a band in the time space using a Biquad filter.
    // This is math that I'm not very familiar with, to be honest.
    float processBiquad(BiquadFilter *filter, float input) {
        float output = filter->b0 * input + 
                      filter->b1 * filter->x1 + 
                      filter->b2 * filter->x2 - 
                      filter->a1 * filter->y1 - 
                      filter->a2 * filter->y2;
        
        if (isnan(output) || isinf(output)) {
            output = 0.0f;
            filter->x1 = filter->x2 = filter->y1 = filter->y2 = 0.0f;
        }
    
        filter->x2 = filter->x1;
        filter->x1 = input;
        filter->y2 = filter->y1;
        filter->y1 = output;
    
        return output;
    }

public:
    float masterGain = 1.5f;
    AudioModule() {
        for (int i = 0; i != NB_BANDS; ++i) {
            bandGains[i].store(1.0f);
            initBiquad(filters[i], coeffs[i]);
        }
    };
    ~AudioModule() = default;

    void setBand(const int band, const float newGain) {
        bandGains[band].store(newGain);
    }

    void setCurve(const float newBandGains[NB_BANDS]) {
        for (int i = 0; i != NB_BANDS; ++i) {
            bandGains[i].store(newBandGains[i]);
        }
    }

    void reset() {
        for (int i = 0; i != NB_BANDS; ++i) {
            bandGains[i].store(1.0f);
        }
    }

    void apply(const float *inBuffer, float *outBuffer, size_t samples)  {
        std::fill(outBuffer, outBuffer+samples, 0.0f);
       
        for (int band = 0; band < NB_BANDS; band++) {
            for (int i = 0; i < samples; i++) {
                outBuffer[i] += processBiquad(&filters[band], inBuffer[i]) * bandGains[band].load() * masterGain;
            }
        }
    }
};
