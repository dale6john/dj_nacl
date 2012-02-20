#include <math.h>
#include <cstdio>
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cassert>
#include <cmath>
#include <stdint.h>
#include <vector>

#include "view.h"
#include "log.h"

using namespace dj;

void View::getCenter(double &x, double &y) {
/*
  double wx = device2logical_x(m_usable_ur.x - m_usable_ll.x);
  double wy = device2logical_y(m_usable_ur.y - m_usable_ll.y);
  x = x0 + wx / 2.0;
  y = y0 + wy / 2.0;
  */
}

void View::center(double x, double y, double scale) {
  // to logical position
  m_scale_x = m_scale_y = scale;
  m_lg_ll.assign(x - double(m_canvas.useableX()) / m_scale_x / 2.0,
                 y - double(m_canvas.useableY()) / m_scale_y / 2.0);
  m_lg_ur.assign(x + double(m_canvas.useableX()) / m_scale_x / 2.0,
                 y + double(m_canvas.useableY()) / m_scale_y / 2.0);
  //theLog.info("v.center  %3.3f,%3.3f  s:%3.3f", x, y, scale);
}

void View::zoom(device_t x, device_t y, double scale) {
  // x and y are _device_, so y is flipped
  // move by physical amount, scale > 1 zoom out
  //theLog.info("v.zoom/ scale in: %3.3f  existing scale(x): %3.3f", scale, m_scale_x);
  if (scale < 1.0 && m_scale_x < 0.03125)
    return;
  if (scale > 1.0 && m_scale_x > 4)
    return;

  Point lu_center = cv2lu(Point(x, y));
  //theLog.info("v.zoom/ center lu: %3.3f %3.3f", lu_center.x, lu_center.y);
  center(lu_center.x, lu_center.y, scale * m_scale_x);
}

void View::move(int32_t dx, int32_t dy) {
  int32_t x = lu2cv_x((m_lg_ll.x + m_lg_ur.x)/2) + dx;
  // y is inverted from _actual_ device units to 'dv' units
  // this is to make the dv origin the LL
  int32_t y = lu2cv_y((m_lg_ll.y + m_lg_ur.y)/2) - dy;
  //theLog.info("v.move %d,%d => %d %d", dx, dy, x, y);
  zoom(x, y, 1.0);
}

void View::border() {
  // get device x for y axis
  //border
  // FIXME: move to canvas??
  Point ph_ll;
  m_canvas.ll(ph_ll);
  Point ph_ur;
  m_canvas.ur(ph_ur);

  int32_t llx = ph_ll.x + 1;
  int32_t lly = ph_ll.y + 1;
  int32_t urx = ph_ur.x - 2;
  int32_t ury = ph_ur.y - 2;
  m_canvas.hline(llx, urx, lly, 0xffff00ff);
  m_canvas.hline(llx, urx, lly+1, 0xffff00ff);
  m_canvas.hline(llx, urx, ury, 0xffff00ff);
  m_canvas.hline(llx, urx, ury-1, 0xffff00ff);

  m_canvas.vline(llx,   lly, ury, 0xffff00ff);
  m_canvas.vline(llx+1, lly, ury, 0xffff00ff);
  m_canvas.vline(urx,   lly, ury, 0xffff00ff);
  m_canvas.vline(urx-1, lly, ury, 0xffff00ff);
}

void View::draw_axis() {
  // get device x for y axis
  // get all four edges in logical units
  // plot axis based on logical units
  Point ph_ll;
  m_canvas.ll(ph_ll);
  Point ph_ur;
  m_canvas.ur(ph_ur);

  //printf("LU: x %3.3f -> %3.3f   y %3.3f -> %3.3f\n",
  //  lu_ll.x, lu_ur.x, lu_ll.y, lu_ur.y);
  double dv0_x = lu2cv_x(0); // canvas units
  double dv0_y = lu2cv_y(0);

  // y0,y1 is logical for lower and upper y bounds
  int32_t xstep = 1;
  int32_t ystep = 1;

  int32_t lu_xmin = int32_t(m_lg_ll.x);
  int32_t lu_xmax = int32_t(m_lg_ur.x);
  int32_t lu_ymin = int32_t(m_lg_ll.y);
  int32_t lu_ymax = int32_t(m_lg_ur.y);
  if ((lu_xmax - lu_xmin) > 100) {
    xstep = 10;
    lu_xmin = (int32_t(m_lg_ll.x) / xstep) * xstep;
    lu_xmax = (int32_t(m_lg_ur.x) / xstep) * xstep;
    ystep = 10;
    lu_ymin = (int32_t(m_lg_ll.y) / ystep) * ystep;
    lu_ymax = (int32_t(m_lg_ur.y) / ystep) * ystep;
  }
  if (m_lg_ll.x < 0.0 && m_lg_ur.x > 0.0) {
    m_canvas.vline(dv0_x, 0, sz_y(), 0xff00ff00);
    for (int32_t ix = lu_ymin; ix <= lu_ymax; ix+=ystep) {
      int32_t dv_y = lu2cv_y(ix);
      //printf("dv_y %d  height %d\n", dv_y, m_canvas.useableY());
      if (dv_y >= 0 && dv_y < m_canvas.useableY() - 1)
        m_canvas.hline(dv0_x - 2, dv0_x + 3, dv_y, 0xffcccccc);
    }
  }
  if (m_lg_ll.y < 0.0 && m_lg_ur.y > 0.0) {
    m_canvas.hline(0, sz_x(), dv0_y, 0xff00ff00);
    for (int32_t iy = lu_ymin; iy <= lu_ymax; iy+=ystep) {
      int32_t dv_x = lu2cv_x(iy);
      //theLog.info("dv_x %d  height %d  dv0_y %d", dv_x, m_canvas.useableX(), dv0_y);
      if (dv_x >= 0 && dv_x < m_canvas.useableX() - 1)
        m_canvas.vline(dv_x, dv0_y - 2, dv0_y + 2, 0xffcccccc);
    }
  }
}

void View::draw(Drawable& gobi) {
  //
  // check bounding box
  // draw shape border (if there)
  // draw shape

  // viewport dimensions are  x0,y0 -> x1,y1
  double x0 = m_lg_ll.x;
  double x1 = m_lg_ur.x;
  double y0 = m_lg_ll.y;
  double y1 = m_lg_ur.y;
  bool filled = gobi.filled();

  Point p0(x0, y0);
  Point p1(x1, y1);
  BoundingBox view(p0);
  view.test(p1);
#if VERBOSE
  gobi.show();
#endif
  gobi.fix();

#if VERBOSE
  printf("view.bb: ");
  view.show();
  printf("gobi.bb:  ");
  gobi.boundingBox().show();
#endif

  BoundingBox intersection;
  if (!view.has_overlap(gobi.boundingBox(), intersection)) {
#if VERBOSE
    printf("no overlap\n");
#endif
    return;
  }
#if VERBOSE
  printf("potential overlap\n");
  printf("itx:     ");
  intersection.show();
#endif

  if (filled)
    draw_filled(gobi, intersection);
  else
    draw_frame(gobi, intersection);
}

void View::draw_frame(Drawable& gobi, BoundingBox& intersection) {
  uint32_t color = 0xffa0a0a0;
  m_canvas.hline(lu2cv_x(intersection.lower_left.x),
                 lu2cv_x(intersection.upper_right.x),
                 lu2cv_y(intersection.lower_left.y), color);
  m_canvas.hline(lu2cv_x(intersection.lower_left.x),
                 lu2cv_x(intersection.upper_right.x),
                 lu2cv_y(intersection.upper_right.y), color);
  m_canvas.vline(lu2cv_x(intersection.lower_left.x),
                 lu2cv_y(intersection.lower_left.y),
                 lu2cv_y(intersection.upper_right.y), color);
  m_canvas.vline(lu2cv_x(intersection.upper_right.x),
                 lu2cv_y(intersection.lower_left.y),
                 lu2cv_y(intersection.upper_right.y), color);
}

void View::draw_filled(Drawable& gobi, BoundingBox& intersection) {
  // for physical lines, iterate each 'y' and get scan from Gob
  // from: intersection.lower_left.y, intersection.upper_right.y
  double miny = intersection.lower_left.y;
  double maxy = intersection.upper_right.y;

  int32_t dv_y0 = lu2cv_y(miny);
  int32_t dv_y1 = lu2cv_y(maxy);
  uint32_t color = gobi.color();
  for (int32_t y = dv_y0; y < dv_y1; y++) {
    double ly = cv2lu_y(y);
#if 0
    // show BB
    m_canvas.hline(logical2device_x(intersection.lower_left.x),
                 logical2device_x(intersection.upper_right.x),
                 y, 0xffd0d0d0);
#endif
    double dx0, dx1;
    gobi.xrange_reset();
    while (gobi.xrange(ly, dx0, dx1)) {
      // x0, x1 back to physical
      int32_t dv_x0 = lu2cv_x(dx0);
      int32_t dv_x1 = lu2cv_x(dx1);
      m_canvas.hline(dv_x0, dv_x1, y, color); // x,y in canvas co-ordinates
    }
  }
}

