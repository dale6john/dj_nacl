#include "rectangle.h"

using namespace dj;

void Rectangle::show() {
  char s[1024];
  show(s);
  printf("%s\n", s);
}

void Rectangle::show_s(char *s) {
  if (m_needs_redraw) fixbb();
  char buf1[120], buf2[120], buf3[120];
  sprintf(s, "rectangle O%s O2%s P%s hdg:%3.1f s:%3.1f w:%3.1f h:%3.1f",
    m_pts[0].show(buf1), m_pts[2].show(buf2), m_pivot.show(buf3), 
      m_heading * 180 / M_PI, m_scale, m_width, m_height
  );
}

void Rectangle::fixbb() {
  // fix m_pts[0] wrt m_origin and m_heading
  m_pts[0].assign(0.0, 0.0);
  m_pts[0].subtract(m_pivot);
  m_pts[0].rotate(m_heading);
  m_pts[0].scale(m_scale);
  m_pts[0].add(m_at);

  // compute line segments, store in p2 .. p5
  double deg90 = M_PI_2;
  double deg180 = M_PI;
  m_pts[1].vector(m_pts[0], m_heading, m_width * m_scale);
  m_pts[2].vector(m_pts[1], m_heading + deg90, m_height * m_scale);
  m_pts[3].vector(m_pts[2], m_heading + deg180, m_width * m_scale);
  m_pts[4].assign(m_pts[0]);

  // establish bounding box
  m_box.lower_left = m_box.upper_right = m_pts[0];
  m_box.test(m_pts[1]);
  m_box.test(m_pts[2]);
  m_box.test(m_pts[3]);

  m_needs_redraw = false;
  m_slopes_wrong = true;
}

void Rectangle::calc_slopes() {
  // calculate slope and intercept of each line segment
  // TODO: this may be the wrong place for this
  Point *pp0 = &m_pts[0];
  Point *pp1 = &m_pts[1];
  uint32_t ix = 0;
  while (pp1 <= &m_pts[4]) {
    double m = 1e6;
    if (fabs(pp1->x - pp0->x) > 1e-6)
      m = (pp1->y - pp0->y) / (pp1->x - pp0->x);
    else
      m = 1000.0;
    m_slopes[ix] = m;
    double b = pp0->y - m * pp0->x;
    m_intercepts[ix] = b;
    ix++;

    pp0 = pp1;
    pp1++;
  }
  m_slopes_wrong = false;
}

bool Rectangle::xrange(double y, double &x0, double &x1) {
  // needs iterator model, one iteration here

  // just return this once per xrange_reset
  if (!m_xr) {
    return false;
  }
  m_xr = false;

  if (m_slopes_wrong)
    calc_slopes();

  Point *pp0 = &m_pts[0];
  Point *pp1 = &m_pts[1];
  if (m_needs_redraw) fixbb();
  double xs[2];
  uint32_t num_pts = 0;
  uint32_t ix = 0;
  while (pp1 <= &m_pts[4]) {
    if ((y < pp0->y) ^ (y < pp1->y)) {
      double m = m_slopes[ix];
      double b = m_intercepts[ix];
      double x = (y - b) / m;
#if VVERBOSE
      char buf[120];
      char buf2[120];
      printf("Rect: x=%3.3f  m=%3.3f  b=%3.3f  y=%3.3f  pp0=%s pp1=%s\n",
          x, m, b, y, pp0->show(buf), pp1->show(buf2));
#endif
      xs[num_pts] = x;
      if (++num_pts == 2) break;
    }
    pp0 = pp1;
    pp1++;
    ix++;
  }
  if (num_pts == 2) {
    if (xs[0] > xs[1]) {
      x0 = xs[1];
      x1 = xs[0];
    } else {
      x0 = xs[0];
      x1 = xs[1];
    }
    return true;
  }
  return false;
}

/*
virtual void Rectangle::transform(
          double grp_heading, double grp_scale, Point& grp_pivot,
          Point& grp_ref, Point& grp_at) {
  // needs to smart enough to apply real-time per instance information
  // and continue

  Point at = inst.at;
  double scale = inst.scale;
  double heading = inst.heading;

  // fix m_pts[0] wrt m_origin and m_heading
  Point* m_pts = inst.pts();
  m_pts[0].assign(m_reference);
  m_pts[0].rotate_about(heading, scale, m_pivot);
  m_pts[0].add(at);


  // apply group transform
  // move by group pivot
  // rotate
  // scale
  // move by group ref
  m_pts[0].rotate_about(grp_heading, grp_scale, grp_pivot);
  m_pts[0].subtract(grp_ref);
  m_pts[0].add(grp_at);
  heading += grp_heading;
  scale *= grp_scale;

  // compute line segments, store in p2 .. p5
  double deg90 = M_PI_2;
  double deg180 = M_PI;  // FIXME ??
  m_pts[1].vector(m_pts[0], heading, m_width * scale);
  m_pts[2].vector(m_pts[1], heading + deg90, m_height * scale);
  m_pts[3].vector(m_pts[2], heading + deg180, m_width * scale);

  // establish bounding box
  bbox.lower_left = bbox.upper_right = m_pts[0];
  bbox.test(m_pts[1]);
  bbox.test(m_pts[2]);
  bbox.test(m_pts[3]);
  //m_needs_redraw = false;
}
*/

