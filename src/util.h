#ifndef UTIL_H
#define UTIL_H

#include <Arduino.h>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

#if DEBUG
#define assert(expr, reason) if (!(expr)) { logf("ASSERTION FAILED: %s", reason); while (1) delay(100); }
#else
#define assert(expr, reason) if (!(expr)) { logf("ASSERTION FAILED: %s", reason); }
#endif

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

static int vasprintf(char** strp, const char* fmt, va_list ap) {
  va_list ap2;
  va_copy(ap2, ap);
  char tmp[1];
  int size = vsnprintf(tmp, 1, fmt, ap2);
  if (size <= 0) {
    strp=NULL;
    return size;
  }
  va_end(ap2);
  size += 1;
  *strp = (char*)malloc(size * sizeof(char));
  return vsnprintf(*strp, size, fmt, ap);
}

void logf(const char *format, ...)
{
  va_list argptr;
  va_start(argptr, format);
  char *buf;
  vasprintf(&buf, format, argptr);
  va_end(argptr);
  Serial.println(buf ? buf : "LOGF MEMORY ERROR");
#if DEBUG
  Serial.flush();
#endif
  free(buf);
}

#define MOD_DISTANCE(a, b, m) (m / 2. - fmod((3 * m) / 2 + a - b, m))

inline int mod_wrap(int x, int m) {
  int result = x % m;
  return result < 0 ? result + m : result;
}

inline float fmod_wrap(float x, int m) {
  float result = fmod(x, m);
  return result < 0 ? result + m : result;
}

class FrameCounter {
  private:
    unsigned long lastPrint = 0;
    long frames = 0;
    long lastClamp = 0;
  public:
    long printInterval = 2000;
    void tick() {
      unsigned long mil = millis();
      long elapsed = MAX(1, mil - lastPrint);
      if (elapsed > printInterval) {
        if (lastPrint != 0) {
          // arduino-samd-core can't sprintf floats??
          // not sure why it's not working for me, I should have Arduino SAMD core v1.8.9
          // https://github.com/arduino/ArduinoCore-samd/issues/407
          logf("Framerate: %i", (int)(frames / (float)elapsed * 1000));
        }
        frames = 0;
        lastPrint = mil;
      }
      ++frames;
    }
    void clampToFramerate(int fps) {
      int delayms = 1000 / fps - (millis() - lastClamp);
      if (delayms > 0) {
        delay(delayms);
      }
      lastClamp = millis();
    }
};

int lsb_noise(int pin, int numbits) {
  // TODO: Use Entropy.h? Probs not needed just to randomize pattern.
  int noise = 0;
  for (int i = 0; i < numbits; ++i) {
    int val = analogRead(pin);
    noise = (noise << 1) | (val & 1);
  }
  return noise;
}

#endif
