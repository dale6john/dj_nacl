#ifndef __point_h__
#define __point_h__

#include <math.h>
#include <stdio.h>
#include <assert.h>

namespace dj {

class Point {
 public:
  Point() {}
  Point(uint32_t xy) : x(xy), y(xy) {}
  Point(double x_, double y_) : x(x_), y(y_) {}
  Point(const Point& p) : x(p.x), y(p.y) {}
  inline void vector(Point& p, double angle, double dist) {
    x = p.x + cos(angle) * dist;
    y = p.y + sin(angle) * dist;
  }
  inline double distance() {
    return sqrt(x * x + y * y);
  }
  inline void unit() {
    scale(1.0/distance());
  }
  inline void add(Point& p) {
    x += p.x;
    y += p.y;
  }
  inline void add(double x_, double y_) {
    x += x_;
    y += y_;
  }
  inline void subtract(Point& p) {
    x -= p.x;
    y -= p.y;
  }
  inline void scale(double scale) {
    x *= scale;
    y *= scale;
  }
  inline void assign(Point& p) {
    x = p.x;
    y = p.y;
  }
  inline void assign(double x_, double y_) {
    x = x_;
    y = y_;
  }
  inline void rotate(double a) {
    double cos_a = cos(a);
    double sin_a = sin(a);
    double x2 = x * cos_a - y * sin_a;
    y = x * sin_a + y * cos_a;
    x = x2;
  }
  inline void rotate_about(double angle, double _scale, Point& pivot) {
    subtract(pivot);
    rotate(angle);
    scale(_scale);
    add(pivot);
  }
  char* show(char * buf) {
    sprintf(buf, "(%4.1f,%4.1f)", x, y);
    return buf;
  }

  double x;
  double y;
};

}

#endif
