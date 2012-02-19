#include "circle.h"

using namespace dj;

void Circle::show() {
  char s[1024];
  show_s(s);
  printf("%s\n", s);
}

void Circle::show_s(char *s) {
  if (m_needs_redraw) fixbb();
#if 0
  char buf1[120], buf2[120], buf3[120];
  sprintf(s, "circle O%s O2%s P%s hdg:%3.1f s:%3.1f w:%3.1f h:%3.1f",
    m_pts[0].show(buf1), m_pts[2].show(buf2), m_pivot.show(buf3), 
      m_heading * 180 / M_PI, m_scale, m_radius, m_radius
  );
#endif
}

void Circle::fixbb() {
  Point p1(m_at.x - m_radius, m_at.y - m_radius);
  Point p2(m_at.x + m_radius, m_at.y + m_radius);
  m_box.assign(p1);
  m_box.test(p2);

  m_needs_redraw = false;
}

double Circle::area() const {
  return 3.14159 * m_radius * m_radius;
}

bool Circle::xrange(double y, double &x0, double &x1) {
  // just return this once per xrange_reset
  //theLog.info("CIRCLE[%3.3f]", y);
  if (!m_xr) {
    return false;
  }
  m_xr = false;

  double dy = m_at.y - y;
  if (dy >= m_radius || dy >= m_radius)
    return false;

  // set x0, x1 to the start and end of interior line, return true
  double use_y_scaled = fabs(dy / m_radius);
  double use_x_scaled = sqrt(1.0 - use_y_scaled * use_y_scaled);
  double use_x = use_x_scaled * m_radius;
  x0 = m_at.x - use_x;
  x1 = m_at.x + use_x;
  //theLog.info("CIRCLE x[%3.1f,%3.1f]", x0,x1);

  return true;
}
