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
  View(Canvas& canvas, double scale) 
      : m_canvas(canvas),
        m_lg_ll(0, 0),
        m_scale_x(scale),
        m_scale_y(scale),
        m_wx(double(canvas.width()) / m_scale_x),
        m_wy(double(canvas.height()) / m_scale_y)
  {
  }
  ~View() {}
 public:
  inline int32_t sz_x() { return m_canvas.width(); }
  inline int32_t sz_y() { return m_canvas.height(); }

  inline double dv2lu_x(int32_t ph_x) {
    return (ph_x - m_canvas.llx()) * m_scale_x + m_lg_ll.x;
  }
  inline double dv2lu_y(int32_t ph_y) {
    return (ph_y - m_canvas.lly()) * m_scale_y + m_lg_ll.y;
  }
  inline Point dv2lu(Point ph) {
    Point pt(dv2lu_x(ph.x), dv2lu_y(ph.y));
    return pt;
  }
  
  inline int32_t lu2dv_x(double lu_x) {
    int32_t ret = int32_t(((lu_x - m_lg_ll.x) * m_scale_x)) + m_canvas.llx();
    //printf("x: (lux:%3.3f => dux:%d\n", lu_x, ret);
    return ret;
  }
  inline int32_t lu2dv_y(double lu_y) {
    int32_t ret = int32_t(((lu_y - m_lg_ll.y) * m_scale_y)) + m_canvas.lly();
    //printf("y: (luy:%3.3f => duy:%d\n", lu_y, ret);
    return ret;
  }
  inline Point lu2dv(int32_t x, int32_t y) {
    Point pt(lu2dv_x(x), lu2dv_y(y));
    return pt;
  }

  void move(int32_t dx, int32_t dy);

  void center(double x, double y, double scale = 1.0);

  void zoom(device_t x, device_t y, double scale);

  void draw_axis();

  void border();

  void draw(Drawable& gobi);

  //void draw(Canvas& canvas, uint32_t step, GobGroup& ggrp);

  void getCenter(double &x, double &y);

 public:
  Canvas& m_canvas;
  Point m_lg_ll; // logical lower left
  Point m_lg_ur; // logical upper right
  double m_scale_x; // multiplier for logical co-ordinates to physical
  double m_scale_y;
  double m_wx;  // (whole canvas) width in logical units
  double m_wy;  // (whole canvas) height in logical units
};

}

#endif
