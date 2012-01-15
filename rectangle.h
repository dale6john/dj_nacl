#ifndef __rectangle_h__
#define __rectangle_h__

#include "gob.h"
#include "point.h"

namespace dj {

/*
 * definition and concrete storage for a rectangle instance
 */
class Rectangle : public Drawable {
 public:
  Rectangle(Point at, double w, double height)
      : Drawable(at, 1.0, 0.0), m_width(w), m_height(height),
        m_slopes_wrong(true)
  {
    m_pivot.assign(w/2, height/2);
  }
  Rectangle(Point pivot, Point at, double w, double height)
      : Drawable(pivot, at, 1.0, 0.0), m_width(w), m_height(height),
        m_slopes_wrong(true)
  {
  }
  Rectangle(Rectangle& other) 
      : Drawable(other.m_pivot, other.m_at, other.m_scale, other.m_heading),
        m_width(other.m_width), m_height(other.m_height),
        m_slopes_wrong(true)
  {
  }
  ~Rectangle() {}

  virtual void show();
  virtual void show(char *s);
  virtual void fixbb(); // establish BB and cache rectangle points
  //virtual void transform(
  //          double grp_heading, double grp_scale, Point& grp_pivot,
  //          Point& grp_ref, Point& grp_at);
  virtual void xrange_reset() { m_xr = true; }
  virtual bool xrange(double y, double &x0, double &x1);
  void calc_slopes();

 private:
  double m_width;
  double m_height;

  bool m_xr;
  bool m_slopes_wrong;
  Point m_pts[5];
  double m_slopes[4];
  double m_intercepts[4];
};

};

#endif
