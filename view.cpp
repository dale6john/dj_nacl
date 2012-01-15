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

using namespace dj;

void View::getCenter(double &x, double &y) {
  x = x0 + m_wx / 2.0;
  y = y0 + m_wy / 2.0;
}

void View::center(double x, double y, double scale) {
  // move to logical location
  x0 = x - m_wx / 2.0;
  y0 = y - m_wy / 2.0;
  m_scale_x *= scale;
  m_scale_y *= scale;
  m_wx = double(m_sz_x) / m_scale_x;
  m_wy = double(m_sz_y) / m_scale_y;
}

void View::zoom(device_t x, device_t y, double scale) {
  // x and y are _device_, so y is flipped
  // move by physical amount, scale > 1 zoom out
  if (scale < 1.0 && m_scale_x < 0.03125)
    return;
  if (scale > 1.0 && m_scale_x > 4)
    return;

  double vx = x0 + m_wx * double(x) / double(m_sz_x);
  double vy = y0 + m_wy * double(m_sz_y - y) / double(m_sz_y);
  m_scale_x *= scale;
  m_scale_y *= scale;
  m_wx = double(m_sz_x) / m_scale_x;
  m_wy = double(m_sz_y) / m_scale_y;
  x0 = vx - m_wx / 2.0;
  y0 = vy - m_wy / 2.0;
}

void View::draw_axis(Canvas& canvas) {
  // get device x for y axis
  double x1 = x0 + m_wx;
  double y1 = y0 + m_wy;
  int32_t dv_x0 = logical2device_x(0);
  int32_t dv_y0 = logical2device_y(0);

  // y0,y1 is logical for lower and upper y bounds
  int32_t xmin = -10;
  int32_t xmax = 10;
  int32_t xstep = 1;
  int32_t ymin = -10;
  int32_t ymax = 10;
  int32_t ystep = 1;
  if (m_wx > 100) {
    xstep = 10;
    xmin = -(m_wx / xstep) * xstep;
    xmax = (m_wx / xstep) * xstep;
    ystep = 10;
    ymin = -(m_wy / ystep) * ystep;
    ymax = (m_wy / ystep) * ystep;
  }
  if (y0 < 0 && y1 > 0) {
    canvas.hline(0, 0xffff, dv_y0, 0xffcccccc);
    for (int32_t ix = xmin; ix <= xmax; ix+=xstep) {
      int32_t dv_y = logical2device_y(ix);
      if (dv_y >= 0 && dv_y < canvas.height())
        canvas.hline(dv_x0 - 1, dv_x0 + 1, dv_y, 0xffcccccc);
    }
  }
  if (x0 < 0 && x1 > 0) {
    canvas.vline(dv_x0, 0, 0xffff, 0xffcccccc);
    for (int32_t iy = ymin; iy <= ymax; iy+=ystep) {
      int32_t dv_x = logical2device_x(iy);
      if (dv_x >= 0 && dv_x < canvas.width())
        canvas.vline(dv_x, dv_y0 - 1, dv_y0 + 1, 0xffcccccc);
    }
  }
}

void View::draw(Canvas& canvas, Drawable& gobi) {
  //
  // check bounding box
  // draw shape border (if there)
  // draw shape

  //double scale = m_scale_x;  // bitmap to physical(device) scale

  // viewport dimensions are  x0,y0 -> x1,y1
  double x1 = x0 + m_wx;
  double y1 = y0 + m_wy;
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

  // for physical lines, iterate each 'y' and get scan from Gob
  // from: intersection.lower_left.y, intersection.upper_right.y
  double miny = intersection.lower_left.y;
  double maxy = intersection.upper_right.y;

  int32_t dv_y0 = logical2device_y(miny);
  int32_t dv_y1 = logical2device_y(maxy);
#if VVERBOSE
  printf("device y range: %d to %d (not y flipped)\n", dv_y0, dv_y1);
#endif
  uint32_t color = gobi.color();
  for (int32_t y = dv_y0; y <= dv_y1; y++) {
    double ly = device2logical_y(y);
#if VVERBOSE
    printf("logical y: %3.3f\t", ly);
#endif
#if 0
    // show BB
    canvas.hline(logical2device_x(intersection.lower_left.x),
                 logical2device_x(intersection.upper_right.x),
                 y, 0xffd0d0d0);
#endif
    double dx0, dx1;
    gobi.xrange_reset();
    while (gobi.xrange(ly, dx0, dx1)) {
#if VVERBOSE
      printf("scan x: %3.3f to %3.3f (logical) (Gob)\t", dx0, dx1);
#endif
      // x0, x1 back to physical
      int32_t dv_x0 = logical2device_x(dx0);
      int32_t dv_x1 = logical2device_x(dx1);
#if VVERBOSE
      printf("physical x: %d to %d\n", dv_x0, dv_x1);
#endif
      //canvas.hline(dv_x0, dv_x1, y, 0xff000000);
      canvas.hline(dv_x0, dv_x1, y, color);
    }
  }
}

#if 0
void View::draw(Canvas& canvas, uint32_t step, Group& ggrp) {
  //
  // check bounding box
  // draw shape border (if there)
  // draw shape

  //double scale = m_scale_x;  // bitmap to physical(device) scale

  // viewport dimensions are  x0,y0 -> x1,y1
  double x1 = x0 + m_wx;
  double y1 = y0 + m_wy;
  Point p0(x0, y0);
  Point p1(x1, y1);
  BoundingBox view(p0);
  view.test(p1);
  ggrp.fix();

#if VERBOSE
  printf("view.bb: ");
  view.show();
  printf("ggrp.bb:  ");
  ggrp.boundingBox().show();
#endif

  BoundingBox intersection;
  if (!view.has_overlap(ggrp.boundingBox(), intersection)) {
    printf("no overlap\n");
    return;
  } else {
    printf("ggrp overlap\n");
    printf("itx0:    ");
    intersection.show();
  }

  // iterate each item
  //std::vector<Gob *>::iterator it;
  Group::iterator it = ggrp.begin();
  for ( ; it != ggrp.end(); ++it) {
    Gob &gob = *(*it);

    BoundingBox intersection2;
    if (!view.has_overlap(gob.boundingBox(), intersection2)) {
      printf("no overlap\n");
      return;
    }
#if VERBOSE
    printf("potential overlap\n");
    printf("itx:     ");
    intersection.show();
#endif

    // for physical lines, iterate each 'y' and get scan from Gob
    // from: intersection.lower_left.y, intersection.upper_right.y
    double miny = intersection.lower_left.y;
    double maxy = intersection.upper_right.y;

    int32_t dv_y0 = logical2device_y(miny);
    int32_t dv_y1 = logical2device_y(maxy);
#if VVERBOSE
    printf("device y range: %d to %d (not y flipped)\n", dv_y0, dv_y1);
#endif
    for (int32_t y = dv_y0; y < dv_y1; y++) {
      double ly = device2logical_y(y);
#if VVERBOSE
      printf("logical y: %3.3f\t", ly);
#endif
      double dx0, dx1;
      if (gob.xrange(ly, dx0, dx1)) {
#if VVERBOSE
        printf("scan x: %3.3f to %3.3f (logical) (ggrp)\t", dx0, dx1);
#endif
        // x0, x1 back to physical
        int32_t dv_x0 = logical2device_x(dx0);
        int32_t dv_x1 = logical2device_x(dx1);
#if VVERBOSE
        printf("physical x: %d to %d\n", dv_x0, dv_x1);
#endif
        canvas.hline(dv_x0, dv_x1, y, 0xff000000);
      }
    }
    printf("\n");
  }
}
#endif

