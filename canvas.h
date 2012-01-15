#ifndef __canvas_h__
#define __canvas_h__

#include "types.h"

/*
 * how to partition the canvas into separate plotting areas like
 * windows.
 */
namespace dj {

class Canvas {
 public:
  Canvas(uint32_t* pixel_bits, device_t width, device_t height)
      : m_px(pixel_bits), m_w(width), m_h(height),
        m_minx(0), m_miny(0), m_maxx(width), m_maxy(height) {}
  ~Canvas() {};

  void usable(uint32_t from_x, uint32_t from_y, uint32_t to_x, uint32_t to_y) {
    // map a given usable area of the canvas to the effects of the
    // canvas drawing primitives.  All output to a given 'window' will
    // be made after a 'usable()' call.
    m_minx = from_x;
    m_miny = from_y;
    m_maxx = to_x;
    m_maxy = to_y;
  }

  inline void set(int32_t x, int32_t y, uint32_t color) {
    // input (inverted) DEVICE co-ordinates
    // FIXME: do we really need all 4 comparisons here?
    if ((y >= m_miny && y < m_maxy) && (x >= m_minx && x < m_maxx))
      m_px[m_w * (m_h - 1 - y) + x] = color;
#if 0
    if (x < m_w && y < m_h)
      m_px[m_w * (m_h - 1 - y) + x] = color;
    else {
      //printf("2/ set overrun x=%d (limit %d) y=%d (limit %d)\n", x, m_w, y, m_h);
      //assert(!"overrun");
    }
#endif
  }
  inline void hline(int32_t x0, int32_t x1, int32_t y, uint32_t color) {
#if 1
    if (x0 < m_minx) x0 = m_minx;
    if (x1 >= m_maxx) x1 = m_maxx - 1;
    if (x0 > x1) return;
#endif
    // assert x1 > x0
    //if (x0 < m_minx) return;
    //if (x1 >= m_maxx) return;
    //if (y >= int32_t(m_h)) return;
    if (y >= m_miny && y < m_maxy) {
#if 0
      // 5.88 sec / 2000 * 10k objects
      for (int32_t xx = x0; xx <= x1; xx++)
        m_px[m_w * (m_h - 1 - y) + xx] = color;
#else
      // 5.69 sec / 2000 * 10k objects
      // 5.44 sec (caching slopes)
      uint32_t* p0 = &m_px[m_w * (m_h - 1 - y) + x0];
      //p0[0] = fix_color(color);
      //p0[x1 - x0 - 1] = fix_color(color);
      //p0[0] = fix_color(color);
      p0[0] = color;
      //p0[x1 - x0 - 1] = color;
      for (int32_t i = 1; i < x1 - x0 - 1; i++)
        *++p0 = color;
#endif
    } else {
      printf("2/ set overrun x=%d (limit %d) y=%d (limit %d)\n", x1, m_w, y, m_h);
      assert(!"overrun");
    }
  }
  inline void vline(int32_t x, int32_t y0, int32_t y1, uint32_t color) {
    // FIXME: not really used now.
    if (y1 >= int32_t(m_h)) y1 = m_h - 1;
    if (y0 < 0) y0 = 0;
    if (y0 >= int32_t(m_h)) return;
    if (y1 >= int32_t(m_h)) return;
    if (x >= int32_t(m_w)) return;
    if (x >= 0 && x < int32_t(m_w)) {
      for (int32_t yy = y0; yy <= y1; yy++)
        m_px[m_w * (m_h - 1 - yy) + x] = color;
    } else {
      printf("3/ set overrun x=%d (limit %d) y=%d (limit %d)\n", x, m_w, y1, m_h);
      assert(!"overrun");
    }
  }
  inline uint32_t get(device_t x, device_t y) const {
    // for testing only
    if (x < m_w && y < m_h)
      return m_px[m_w * (m_h - 1 - y) + x];
    else {
      printf("4/ get overrun x=%d (limit %d) y=%d (limit %d)\n", x, m_w, y, m_h);
      assert(!"overrun");
    }
    return 0;
  }
  inline int32_t height() { return m_h; }
  inline int32_t width() { return m_w; }
  void clear() {
    memset(m_px, 0xff, sizeof(uint32_t) * m_w * m_h);
  }
 public: // FIXME
  uint32_t* m_px;
 private:
  device_t m_w;
  device_t m_h;

  int32_t m_minx;
  int32_t m_miny;
  int32_t m_maxx;
  int32_t m_maxy;
};

}

#endif
