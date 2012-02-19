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

  inline double cv2dv_x(int32_t cv_x) {
    return cv_x + m_canvas.llx();
  }
  inline double cv2dv_y(int32_t cv_y) {
    return cv_y + m_canvas.lly();
  }

  inline double cv2lu_x(int32_t ph_x) {
    //double ret = (ph_x - m_canvas.llx()) * m_scale_x + m_lg_ll.x;
    double ret = ph_x / m_scale_x + m_lg_ll.x;
    //theLog.info("cv2lu_x(%d) => %3.3f", ph_x, ret);
    //theLog.info("cv2lu_x(%d) s:%3.3f llx:%3.3f => %3.3f", ph_x, m_scale_x, m_lg_ll.x, ret);
    return ret;
  }
  inline double cv2lu_y(int32_t ph_y) {
    double ret = ph_y / m_scale_y + m_lg_ll.y;
    //double ret = (ph_y - m_canvas.lly()) * m_scale_y + m_lg_ll.y;
    //theLog.info("cv2lu_y(%d) s:%3.3f lly:%3.3f => %3.3f", ph_y, m_scale_y, m_lg_ll.y, ret);
    return ret;
  }
  inline Point cv2lu(Point ph) {
    Point pt(cv2lu_x(ph.x), cv2lu_y(ph.y));
    return pt;
  }
  
  inline int32_t lu2cv_x(double lu_x) {
    //int32_t ret = int32_t(((lu_x - m_lg_ll.x) * m_scale_x)) + m_canvas.llx();
    int32_t ret = int32_t(((lu_x - m_lg_ll.x) * m_scale_x));
    //printf("x: (lux:%3.3f => dux:%d\n", lu_x, ret);
    return ret;
  }
  inline int32_t lu2cv_y(double lu_y) {
    //int32_t ret = int32_t(((lu_y - m_lg_ll.y) * m_scale_y)) + m_canvas.lly();
    int32_t ret = int32_t(((lu_y - m_lg_ll.y) * m_scale_y));
    //printf("y: (luy:%3.3f => duy:%d\n", lu_y, ret);
    return ret;
  }
  inline Point lu2cv(int32_t x, int32_t y) {
    Point pt(lu2cv_x(x), lu2cv_y(y));
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

  const char * debug() {
    sprintf(m_debug, "LL(%3.1f,%3.1f) UR(%3.1f,%3.1f) s(%3.2f,%3.2f)",
      m_lg_ll.x, m_lg_ll.y, m_lg_ur.x, m_lg_ur.y, m_scale_x, m_scale_y);
    return m_debug;
  }

 public:
  Canvas& m_canvas;
  Point m_lg_ll; // logical lower left
  Point m_lg_ur; // logical upper right
  double m_scale_x; // multiplier for logical co-ordinates to physical
  double m_scale_y;
  double m_wx;  // (whole canvas) width in logical units
  double m_wy;  // (whole canvas) height in logical units
  char m_debug[1024];
};

}

#endif
