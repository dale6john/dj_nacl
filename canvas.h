#ifndef __canvas_h__
#define __canvas_h__

#include "types.h"
#include "point.h"
#include "log.h"

/*
 * how to partition the canvas into separate plotting areas like
 * windows.
 */
namespace dj {

class Canvas {
 public:
  Canvas(uint32_t* pixel_bits, device_t width, device_t height)
      : m_px(pixel_bits), m_w(width), m_h(height),
        m_minx(0), m_miny(0), m_maxx(width), m_maxy(height),
        m_clips(0) {}
  ~Canvas() {};

  void setPixelBits(uint32_t* p) { m_px = p; }

  void usable(uint32_t from_x, uint32_t from_y, uint32_t to_x, uint32_t to_y) {
    // map a given usable area of the canvas to the effects of the
    // canvas drawing primitives.  All output to a given 'window' will
    // be made after a 'usable()' call.
    m_minx = from_x;
    m_miny = from_y;
    m_maxx = to_x;
    m_maxy = to_y;
  }

  inline int32_t middleX() { return (m_maxx - m_minx) / 2 + m_minx; }
  inline int32_t middleY() { return (m_maxy - m_miny) / 2 + m_miny; }
  inline int32_t useableX() { return m_maxx - m_minx; }
  inline int32_t useableY() { return m_maxy - m_miny; }
  inline void ll(Point& pt) { pt.assign(m_minx, m_miny); }
  inline int32_t llx() { return m_minx; }
  inline int32_t lly() { return m_miny; }
  inline int32_t urx() { return m_maxx; }
  inline int32_t ury() { return m_maxy; }
  inline void ur(Point& pt) { pt.assign(m_maxx, m_maxy); }
  inline uint32_t clips() const { return m_clips; }

  inline void border(uint8_t pixels, uint32_t color) {
    int32_t dx = m_maxx - m_minx;
    int32_t dy = m_maxy - m_miny;
    //theLog.info("drawing border x:%d %d  y:%d %d", m_minx, m_maxx, m_miny, m_maxy);
    for (uint32_t i = 0; i < pixels; i++) {
      hline(0, dx - 1, i, color);
      hline(0, dx - 1, dy - 1 - i, color);
      vline(i, 1, dy - 1, color);
      vline(dx - 1 - i, 1, dy - 1, color);
    }
    m_border = pixels;
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
    // inputs are canvas units
    int32_t x0fix = x0 + m_minx;
    int32_t x1fix = x1 + m_minx;
    int32_t yfix = y + m_miny; // translate from canvas co-ordinates to physical
#if 1
    if (x0fix < m_minx + m_border) x0fix = m_minx + m_border;
    if (x1fix >= m_maxx - m_border) x1fix = m_maxx - 1 - m_border;
    if (x0fix > x1fix) return;
#endif
    if (yfix < m_maxy - m_border) {
    //if (yfix >= m_miny && yfix < m_maxy) 
#if 0
      // 5.88 sec / 2000 * 10k objects
      for (int32_t xx = x0fix; xx <= x1fix; xx++)
        m_px[m_w * (m_h - 1 - yfix) + xx] = color;
#else
      // 5.69 sec / 2000 * 10k objects
      // 5.44 sec (caching slopes)
      //
      // FLIPPING Y to DEVICE UNITS from CANVAS UNITS
      //
      uint32_t* p0 = &m_px[m_w * (m_h - 1 - yfix) + x0fix];
      //p0[0] = fix_color(color);
      //p0[x1fix - x0fix - 1] = fix_color(color);
      //p0[0] = fix_color(color);
      p0[0] = color;
      //p0[x1fix - x0fix - 1] = color;
      for (int32_t i = 1; i < x1fix - x0fix - 1; i++)
        *++p0 = color;
#endif
    } else {
      //theLog.info("clip: y=%d (unflipped)", yfix);
      m_clips++;
#if PARANOID
      printf("2/ set overrun x=%d (limit %d) yfix=%d (limit %d %d->%d)\n", x1fix, m_w, yfix, m_h, m_miny, m_maxy);
      assert(!"overrun");
#endif
    }
  }
  inline void vline(int32_t x, int32_t y0, int32_t y1, uint32_t color) {
    // inputs are canvas units
    int32_t xfix = x + m_minx;
    int32_t y0fix = y0 + m_miny;
    int32_t y1fix = y1 + m_miny;

    if (y1fix >= int32_t(m_h)) y1fix = m_h - 1;
    if (y0fix < 0) y0fix = 0;
    if (y0fix >= int32_t(m_h)) return;
    if (y1fix >= int32_t(m_h)) return;
    if (xfix >= int32_t(m_w)) return;
    if (xfix >= 0 && xfix < int32_t(m_w)) {
      for (int32_t yy = y0fix; yy <= y1fix; yy++)
        m_px[m_w * (m_h - 1 - yy) + xfix] = color;
    } else {
      printf("3/ set overrun xfix=%d (limit %d) y=%d (limit %d)\n", xfix, m_w, y1fix, m_h);
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

 private:
  uint32_t* m_px;

  device_t m_w;
  device_t m_h;

  int32_t m_minx;
  int32_t m_miny;
  int32_t m_maxx;
  int32_t m_maxy;
  uint8_t m_border;
  uint32_t m_clips;
};

class CanvasSet {
 // FIXME: is-a canvas??
 public:
  CanvasSet(uint32_t n) { 
    m_c = new Canvas*[n];
    m_tags = new uint32_t[n];
    m_used = 0; 
    for (uint32_t i = 0; i < n; i++)
      m_c[i] = NULL;
  }
  ~CanvasSet() {
    delete [] m_c;
    delete [] m_tags;
  }
  void add(Canvas& canvas, uint32_t tag) {
    m_tags[m_used] = tag;
    m_c[m_used++] = &canvas;
  }
  void setPixelBits(uint32_t *px) {
    for (uint32_t i = 0; i < m_used; i++)
      if (m_c[i])
        m_c[i]->setPixelBits(px);
  }
  int32_t getCanvasId(int32_t x, int32_t y, int32_t& tx, int32_t& ty) {
    theLog.info("getCanvasId(%d %d)", x, y);
    for (uint32_t i = 0; i < m_used; i++) {
      int32_t y_inv = m_c[i]->height() - y;
      if (m_c[i]) {
        /*
        theLog.info("i %d (%d,%d) x(%d %d) y(%d %d)", i, x, y_inv,
          m_c[i]->llx(),
          m_c[i]->urx(),
          m_c[i]->lly(),
          m_c[i]->ury()
        );
        */
        if (!(x < m_c[i]->llx() || x > m_c[i]->urx() || y_inv < m_c[i]->lly()
            || y_inv > m_c[i]->ury())) {
          // it's in this one
          tx = x - m_c[i]->llx();
          ty = y_inv - m_c[i]->lly();
          return m_tags[i];
        }
      }
    }
    return -1;
  }
 private:
  Canvas** m_c;
  uint32_t* m_tags;
  uint32_t m_used;
};

}

#endif
