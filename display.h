#ifndef __DJ_display__h__
#define __DJ_display__h__

#include "ascii.h"
#include "view.h"
#include "bitmap.h"

namespace dj {

  class Ascii : public Bitmap {
   public:
    Ascii() : m_width(0), m_height(0) {}
    Ascii(const uint32_t* v, uint32_t width, uint32_t height)
        : Bitmap(v), m_width(width), m_height(height) {}
    ~Ascii() {}

   public:
    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    inline uint32_t at(uint32_t x, uint32_t y, uint32_t v) const {
      // assert m_width
      return Bitmap::at(v * m_width + x, y, 0);
    }
    // bitmap constructor
    Bitmap &digit(uint8_t digit) {
      // when new digit
      if (m_digits[digit].steps() == 0) {
        printf("recalc\n");
        uint32_t * buf = new uint32_t[ m_width * m_height ];
        memset(buf, '\0', sizeof(buf));
        m_digits[digit] = Bitmap(buf, 1, m_width, m_height);
        for (uint32_t x = 0; x < m_width; x++) {
          for (uint32_t y = 0; y < m_height; y++) {
            m_digits[digit].set(x, y, 0, Bitmap::at(digit * m_width + x, y, 0));
          }
        }
      }
      return m_digits[digit];
    }
    void draw(Canvas& canvas, int8_t digit, int32_t x_, int32_t y_, uint32_t color) {
      uint32_t base_r = (color & 0xff0000) >> 16;
      uint32_t base_g = (color & 0xff00) >> 8;
      uint32_t base_b = (color & 0xff);
      if (digit == '\0') digit = '?';
      if (digit < ' ') digit = '~';
      if (digit > '~') digit = '~';
      for (uint32_t x = 0; x < m_width; x++) {
        for (uint32_t y = 0; y < m_height; y++) {
          uint32_t c = Bitmap::at(15 + (digit - ' ') * m_width + x, y, 0);
          uint32_t r = (c & 0xff0000) >> 16;
          uint32_t g = (c & 0xff00) >> 8;
          uint32_t b = (c & 0xff);
          if (r > base_r) r = base_r;
          if (g > base_g) g = base_g;
          if (b > base_b) b = base_b;
          c = (c & 0xff000000) + (r << 16) + (g << 8) + b;
          if (r > 0x10 || g > 0x10 || b > 0x10)
            canvas.set(x + x_, y + y_, c);
          else
            canvas.set(x + x_, y + y_, 0xff000000);
        }
      }
    }
    void draw_s(Canvas& canvas, const char *s, int32_t x_, int32_t y_, uint32_t color) {
      const char *p = s;
      while (*p) {
        draw(canvas, *p, x_, y_, color);
        x_ += 10;
        p++;
      }
    }

   private:
    uint32_t m_width;
    uint32_t m_height;
    Bitmap m_digits[10];
  };


  class DisplayContext {
   public:
    DisplayContext(View* v, Ascii *a) 
        : m_view(v), m_ascii(a) {}
   public:
    View* m_view;
    Ascii* m_ascii;
  };
  
}

#endif
