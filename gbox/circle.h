#ifndef __circle_h__
#define __circle_h__

#include "gob.h"
#include "point.h"
#include "log.h"

namespace dj {

/*
 * definition and concrete storage for a circle instance
 */
class Circle : public Drawable {
 public:
  Circle(Point at, double r)
      : Drawable(at, 1.0, 0.0), m_radius(r)
  {
    m_pivot.assign(r/2, r/2);
    //theLog.info("CIRCLE CONS[%3.3f, %3.3f  r:%3.3f]", at.x, at.y, m_radius);
  }
  Circle(Point pivot, Point at, double w, double height)
      : Drawable(pivot, at, 1.0, 0.0), m_radius(w)
  {
  }
  Circle(Circle& other) 
      : Drawable(other.m_pivot, other.m_at, other.m_scale, other.m_heading),
        m_radius(other.m_radius)
  {
  }
  ~Circle() {}

  virtual void show();
  virtual void show_s(char *s);
  virtual void fixbb();
  virtual void xrange_reset() { m_xr = true; }
  virtual bool xrange(double y, double &x0, double &x1);
  virtual double area() const;

 private:
  double m_radius;

  bool m_xr;
};

};

#endif
