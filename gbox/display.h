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
          // plus one below for funky offset into font-bitmap
          uint32_t c = Bitmap::at(15 + (digit - ' ') * m_width + x + 1, y, 0);
          uint32_t r = (c & 0xff0000) >> 16;
          uint32_t g = (c &   0xff00) >> 8;
          uint32_t b = (c &     0xff);
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
    void draw_s2(Canvas& canvas, const char *s, int32_t x_, int32_t y_, uint32_t color) {
      const char *p = s;
      uint32_t x0 = x_;
      int32_t cx = -1;
      int32_t cy = -1;
      uint8_t ct = 0;
      bool selected = false;
      while (*p) {
        if ((*p) == 13) {
          x_ = x0;
          y_ -= 20;
          selected = false;
        } else if((*p) == '*') {
          ct = *++p;
          if (ct == '(') {
            selected = true;
          } else if (ct == ')') {
            selected = false;
          } else {
            cx = x_; cy = y_;
          }
        } else {
          uint32_t c = color;
          if (selected)
            c = 0xfff1f1f1;
            //c ^= 0x0000ffff;
          draw(canvas, *p, x_, y_, c);
          x_ += 10;
        }
        p++;
      }
      if (cx >= 0) {
        if (ct == '+') {
          uint32_t mask[] = { 0x3ff8, 0x3ff0, 0x1fe0, 0x0e00, 0x0c00, 0x0800 };
          for (uint32_t y2 = 0; y2 < 6; y2++) {
            uint32_t m = mask[y2];
            for (uint32_t x2 = 0; x2 < 16; x2++) {
              if (m & 0x8000)
                canvas.set(cx + x2 - 5, cy + y2, 0xff7f7f7f);
              m <<= 1;
            }
          }
        } else if (ct == '-') {
          for (uint32_t x2 = 0; x2 < 10; x2++) {
            for (uint32_t y2 = 0; y2 < 2; y2++) {
              canvas.set(cx + x2, cy + y2, 0xff7f7f7f);
            }
          }
        } else {
        }
      }
    }
    void draw_s(Canvas& canvas, const char *s, int32_t x_, int32_t y_, uint32_t color) {
      const char *p = s;
      uint32_t x0 = x_;
      while (*p) {
        if ((*p) == 13) {
          x_ = x0;
          y_ -= 20;
        } else {
          draw(canvas, *p, x_, y_, color);
          x_ += 10;
        }
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
