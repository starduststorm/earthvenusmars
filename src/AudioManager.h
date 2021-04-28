#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

// #include <ArduinoSound.h>
#include <Adafruit_ZeroI2S.h>
#include <Adafruit_ZeroFFT.h>

#include <vector>
#include "util.h"

using namespace std;

struct FFTFrame {
  vector<int> spectrum;
  unsigned int amplitude;
};

Adafruit_ZeroI2S i2s = Adafruit_ZeroI2S();
class FFTProcessing {
public:
  static const int sampleRate = 16000;
  const int bitsPerSample = 32;
  
  // size of the FFT to compute
  static const int fftSize = 64;

  // size of the spectrum output, which is half of FFT size
  static const int spectrumSize = fftSize / 2;
  static const int ignoreBins = 2; // ignore the first n bins of the fft

private:
  vector<int> spectrum;
  
public:
  FFTProcessing() {
    startAudio();
  }

  ~FFTProcessing() {
    endAudio();
  }

  void startAudio() {
    static bool once = true;
    if (once) {
      loglf("trying to initialize i2s... ");
      assert(bitsPerSample == 32, "using I2S_32_BIT but not 32 bps");
      if (!i2s.begin(I2S_32_BIT, sampleRate)) {
        logf("Failed to initialize I2S input");
        while(1) delay(10);
      }
      logf("done");
      fftBinsLog();
      once = false;
    }
    loglf("Enable audio rx... ");
    i2s.enableRx();
    logf("done");
  }

  void endAudio() {
    loglf("Disable audio rx... ");
    i2s.disableRx(); // FIXME: This is a no-op inside Adafruit_ZeroI2S on samd21. any reason we actually can't disable it?
    logf("done");
  }

  bool fftAvailable() {
    return i2s.rxReady();
  }

  FFTFrame fftUpdate(unsigned int avgSamples=0) {
    int16_t data[fftSize];
    for(int i=0; i<fftSize; i++) {
      int32_t sample = 0;
      while (!sample) {
        int32_t left=0, right=0;
        if (i2s.rxReady())
          i2s.read(&left, &right); 
        sample = left?:right;
      }
      // convert to 16 bit since our SPH0645LM4H-B supports 18 bits & fills the lower bits with zeros
      // but the ZeroFFT only supports 16 bit
      sample >>= bitsPerSample-16;
      data[i] = (uint16_t)sample;
    }
    ZeroFFT(data, fftSize);

    // guess of noise to subtract based on quiet-room readings
    static const int16_t noise[fftSize] = {0, 0, 4, 4, 2, 2, 3, 3, 2, 2, 1, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 2};
    for (int i = ignoreBins; i < spectrumSize; ++i) {
      data[i] = max(0, data[i] - noise[i]);
    }

    spectrum.clear();
    for(int i=ignoreBins; i<spectrumSize; i++) {
      spectrum.push_back(data[i]);
    }
    FFTFrame frame;
    frame.spectrum = spectrum;
    frame.amplitude = data[0];
    return frame;
  }

  void fftLog(vector<int> &spec) {
    for (int level : spectrum) {
      if (level > 0) {
        loglf("%5i ", level);
      } else {
        loglf("    - ");
      }
    }
    Serial.println();
  }

  void fftLog() {
    vector<int> spec = fftUpdate().spectrum;
    fftLog(spec);
  }

  void fftBinsLog() {
    logf("FFT BINS:");
    for (int i = 0; i < spectrumSize; ++i) {
      uint16_t fc = FFT_BIN(i, sampleRate, fftSize);
      int binSize = sampleRate/fftSize;
      logf("  %i: %i - %iHz", i, fc, fc + binSize-1);
    }
  }
};

#endif
