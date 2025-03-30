// https://github.com/0015/ThatProject/blob/master/ESP32_MICROPHONE/ESP32_INMP441_SETUP_ESP-2.X/ESP32_INMP441_SETUP_ESP-2.X.ino
#include <driver/i2s.h>
#include <arduinoFFT.h>

// INMP441 Microphone pins
#define I2S_WS 26
#define I2S_SD 25
#define I2S_BCK 27
#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 10000
#define BufferLen 256

// PCM5102 Digital to Analog Converter (DAC) pins
#define I2S_DOUT 22  // Data out from ESP32 to PCM5102 DIN pin
// Note: PCM5102 will share BCK and WS pins with the microphone
// (I2S_BCK, I2S_WS) as they're on the same I2S bus

// Create FFT instance and buffers globally
double vReal[BufferLen];
double vImag[BufferLen];
int16_t stereoOutBuffer[BufferLen * 2]; // For returning stereo data

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, BufferLen, SAMPLE_RATE); /* Create FFT object */

int16_t sBuffer[BufferLen];

void setup() {
  Serial.begin(115200);
  Serial.println("Setup I2S ...");

  delay(1000);
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
  delay(500);
}


void FFTAndUndo(int16_t* inBuffer, int samples) {
  for (int i = 0; i < samples; i++) {
    vReal[i] = (double)inBuffer[i];
    vImag[i] = 0.0;
  }
  
  //FFT.windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  
  FFT.compute(vReal, vImag, samples, FFT_FORWARD);
  
  FFT.compute(vReal, vImag, samples, FFT_REVERSE);

  for (int i = 0; i < samples; i++) {
    double scaled = (double)vReal[i];
  }
}

// Add this test function to generate a simple sine wave
void testDAC() {
  // Create a simple sine wave
  int16_t stereoBuffer[BufferLen * 2];
  
  for (int i = 0; i < BufferLen; i++) {
    int16_t sample = sin(i * 0.05) * 10000;  // Sine wave with good amplitude
    stereoBuffer[i*2] = sample;     // Left channel
    stereoBuffer[i*2+1] = sample;   // Right channel
  }
  
  // Send sine wave to DAC
  size_t bytesOut = 0;
  i2s_write(I2S_PORT, stereoBuffer, BufferLen * 2 * sizeof(int16_t), &bytesOut, 10);
}


/* unsigned long last_end = millis(); */

void loop() {
  size_t bytesIn = 0;
  int32_t samples[BufferLen];  // Temporary buffer for 32-bit samples
  
  esp_err_t result = i2s_read(I2S_PORT, samples, BufferLen * sizeof(int32_t), &bytesIn, 10);
  
  if (result == ESP_OK && bytesIn > 0) {
    int samples_read = bytesIn / sizeof(int32_t);

    // Convert 32-bit samples to 16-bit
    for(int i = 0; i < samples_read; i++) {
      sBuffer[i] = samples[i] >> 16;

    }

    // Apply FFT filtering with reduced gain
    FFTAndUndo(sBuffer, samples_read);

    // Prepare stereo output
    for(int i = 0; i < samples_read; i++) {    
      stereoOutBuffer[i*2] = sBuffer[i];
      stereoOutBuffer[i*2+1] = sBuffer[i];
    }
    
    // Write to DAC
    size_t bytesOut = 0;
    result = i2s_write(I2S_PORT, stereoOutBuffer, samples_read * 2 * sizeof(int16_t), &bytesOut, 10);
  }
}

void i2s_install() {
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(32),  // Changed to 32-bit
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,   // Changed to mono input
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,      // Reduced for lower latency
    .dma_buf_len = 64,       // Smaller chunks
    .use_apll = true         // Use better clock
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin(){
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK,     // Bit Clock
    .ws_io_num = I2S_WS,       // Word Select
    .data_out_num = I2S_DOUT,  // Data Output to DAC
    .data_in_num = I2S_SD      // Data Input from Mic
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}