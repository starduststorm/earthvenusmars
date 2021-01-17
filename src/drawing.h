
#ifndef DRAWING_H
#define DRAWING_H

#include <stack>
#include <FastLED.h>

// Workaround for linker issues when using copy-constructors for DrawStyle struct (since something is built with -fno-exceptions)
// https://forum.pjrc.com/threads/57192-Teensy-4-0-linker-issues-with-STL-libraries
extern "C"{
  int __exidx_start(){ return -1;}
  int __exidx_end(){ return -1; }
}
//

enum BlendMode {
  blendSourceOver, blendBrighten, blendDarken, /* add blending? but how to encode alpha? need CRGBA buffers, probs not worth it with current resolution */
};

struct DrawStyle {
public:
  BlendMode blendMode = blendSourceOver;
  bool wrap = false;
};

template<class PixelType, class PixelSetType>
class CustomDrawingContext {
private:
  std::stack<DrawStyle> styleStack;

  void set_px(CustomDrawingContext<PixelType, PixelSetType> &otherContext, PixelType src, int index, BlendMode blendMode) {
    switch (blendMode) {
      case blendSourceOver:
        otherContext.leds[index] = src;
        break;
      case blendBrighten: {
        PixelType dst = leds[index];
        otherContext.leds[index] = PixelType(max(src.r, dst.r), max(src.g, dst.g), max(src.b, dst.b));
        break;
      }
      case blendDarken: {
        PixelType dst = leds[index];
        otherContext.leds[index] = PixelType(min(src.r, dst.r), min(src.g, dst.g), min(src.b, dst.b));
      }
    }
  }
public:
  DrawStyle drawStyle;
  int width, height;
  PixelSetType &leds;
  CustomDrawingContext(PixelSetType &leds, unsigned width, unsigned height) : leds(leds) {
    this->width = width;
    this->height = height;
  }
  
  void blendMode(BlendMode mode) {
    drawStyle.blendMode = mode;
  }
  
  void pushStyle() {
    styleStack.push(drawStyle);
    drawStyle = DrawStyle();
  }
  
  void popStyle() {
    assert(!styleStack.empty(), "No style to pop");
    drawStyle = styleStack.top();
    styleStack.pop();
  }

  void blendIntoContext(CustomDrawingContext<PixelType, PixelSetType> &otherContext, BlendMode blendMode) {
    assert(otherContext.leds.size() == this->leds.size(), "context blending requires same-size buffers");
    for (int i = 0; i < leds.size(); ++i) {
      set_px(otherContext, leds[i], i, blendMode);
    }
  }
};

/* Floating-point pixel buffer support */

typedef struct FCRGB {
  union {
    struct {
      union {
        float r;
        float red;
      };
      union {
        float g;
        float green;
      };
      union {
        float b;
        float blue;
      };
    };
    float raw[3];
  };
public:
  FCRGB() { }
  FCRGB(CRGB color) : red(color.r), green(color.g), blue(color.b) { }
  FCRGB(float r, float g, float b) : red(r), green(g), blue(b) { }
  inline float& operator[] (uint8_t x) __attribute__((always_inline)) {
    return raw[x];
  }
} FCRGB;

template<int SIZE>
class FCRGBArray {
  FCRGB entries[SIZE];
public:
  inline FCRGB& operator[] (uint16_t x) __attribute__((always_inline)) {
    return entries[x];
  };
};

template<unsigned WIDTH, unsigned HEIGHT, class PixelType, class PixelSetType>
struct CustomPixelBuffer {
  CRGBArray<WIDTH*HEIGHT> leds;
  CustomDrawingContext<PixelType, PixelSetType> ctx;
  CustomPixelBuffer() : ctx(leds, WIDTH, HEIGHT) { }
};

#endif
