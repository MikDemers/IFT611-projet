#pragma once

void bandMagnitudes(float* valueBuffer, size_t valueLength, float* bandMagnitudes, std::pair<double, double>* bandsRange) {
    int fftSize = valueLength / 2;
    float binResolution = (float)SAMPLE_RATE / fftSize;

    // Reset band magnitudes
    std::fill(bandMagnitudes, bandMagnitudes+NB_BANDS, 0.0f);

    float bandSum = 0.0f;

    // Process each band
    for (int i = 0; i < NB_BANDS; ++i) {
        // Calculate bin range for this band
        int startIndex = (int)std::round(bandsRange[i].first / binResolution);
        int endIndex = (int)std::round(bandsRange[i].second / binResolution);
        int startBin = std::max(0, startIndex);
        int endBin = std::min(fftSize / 2, endIndex);
        int numBins = endBin - startBin;

        // Process bins in this band
        for (int j = startBin; j < endBin; ++j) {
            int realIdx = j * 2;
            int imagIdx = j * 2 + 1;

            // Calculate magnitude for visualization
            bandSum += sqrt(valueBuffer[realIdx] * valueBuffer[realIdx] + 
                            valueBuffer[imagIdx] * valueBuffer[imagIdx]);
        }

        // Normalize for visualization
        if (numBins > 0) {
            bandMagnitudes[i] = bandSum / numBins;
        }
    }
}
