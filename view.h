#ifndef __view__h__
#define __view__h__

#include "canvas.h"
#include "point.h"
#include "types.h"
#include "gob.h"

namespace dj {

class View {
 public:
  static const uint32_t MAX_APPORTIONMENT = 200;
  View(device_t sz_x, device_t sz_y, double scale) 
      : m_sz_x(sz_x),
        m_sz_y(sz_y),
        x0(0),
        y0(0),
        m_scale_x(scale),
        m_scale_y(scale),
        m_wx(double(m_sz_x) / m_scale_x),
        m_wy(double(m_sz_y) / m_scale_y)
  {
  }
  ~View() {}
 public:
  inline void device2logical(device_t x, device_t y, double &lx, double &ly) {
    lx = x0 + m_wx * double(x) / double(m_sz_x);
    ly = y0 + m_wy * double(y) / double(m_sz_y);
  }
  inline double device2logical_x(device_t x) {
    return x0 + m_wx * double(x) / double(m_sz_x);
  }
  inline double device2logical_y(device_t y) {
    return y0 + m_wy * double(y) / double(m_sz_y);
  }

  inline void logical2device(double lx, double ly, int32_t& dv_x, int32_t& dv_y) {
    dv_y = ((ly - y0) / m_wy) * double(m_sz_y);
    dv_x = ((lx - x0) / m_wx) * double(m_sz_x);
  }
  inline int32_t logical2device_x(double x) {
    return ((x - x0) / m_wx) * double(m_sz_x);
  }
  inline int32_t logical2device_y(double y) {
    return ((y - y0) / m_wy) * double(m_sz_y);
  }

  void move(int32_t dx, int32_t dy) {
    // move by physical amount
    x0 += double(dx) / m_scale_x;
    y0 -= double(dy) / m_scale_y;
  }
  void center(double x, double y, double scale = 1.0);

  void zoom(device_t x, device_t y, double scale);

  void draw_axis(Canvas& canvas);

  void draw(Canvas& canvas, Drawable& gobi);

  //void draw(Canvas& canvas, uint32_t step, GobGroup& ggrp);

  void getCenter(double &x, double &y);

 public:
  device_t m_sz_x; // physical dimensions of canvas
  device_t m_sz_y;
  double x0; // left
  double y0; // lower
  double m_scale_x; // multiplier for logical co-ordinates to physical
  double m_scale_y;
  double m_wx;  // width in logical units
  double m_wy;  // height in logical units
};

}

#endif
