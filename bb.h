#ifndef __bounding_box__
#define __bounding_box__ 1

#include "point.h"

namespace dj {

class BoundingBox {
 public:
  BoundingBox() {}
  BoundingBox(Point p) : lower_left(p), upper_right(p) {}

  Point lower_left;
  Point upper_right;
 
  void assign(BoundingBox& box) {
    lower_left.x  = box.lower_left.x;
    lower_left.y  = box.lower_left.y;
    upper_right.x = box.lower_left.x;
    upper_right.y = box.lower_left.y;
  }
  void assign(Point& p) {
    lower_left.x = p.x;
    lower_left.y = p.y;
    upper_right.x = p.x;
    upper_right.y = p.y;
  }
  void test(Point& p) {
    if (p.x < lower_left.x)  lower_left.x = p.x;
    if (p.y < lower_left.y)  lower_left.y = p.y;
    if (p.x > upper_right.x) upper_right.x = p.x;
    if (p.y > upper_right.y) upper_right.y = p.y;
  }
  bool has_overlap(BoundingBox other, BoundingBox& itx) {
    if (other.upper_right.x < lower_left.x) return false;
    if (other.lower_left.x > upper_right.x) return false;
    if (other.upper_right.y < lower_left.y) return false;
    if (other.lower_left.y > upper_right.y) return false;

    if (lower_left.x < other.lower_left.x)
      itx.lower_left.x = other.lower_left.x;
    else
      itx.lower_left.x = lower_left.x;
    if (lower_left.y < other.lower_left.y)
      itx.lower_left.y = other.lower_left.y;
    else
      itx.lower_left.y = lower_left.y;
    if (upper_right.x > other.upper_right.x)
      itx.upper_right.x = other.upper_right.x;
    else
      itx.upper_right.x = upper_right.x;
    if (upper_right.y > other.upper_right.y)
      itx.upper_right.y = other.upper_right.y;
    else
      itx.upper_right.y = upper_right.y;

    return true;
  }
  void outer_box(BoundingBox other) {
    printf("OB this: "); show();
    printf("OB other:"); other.show();
    if (lower_left.x > other.lower_left.x)
      lower_left.x = other.lower_left.x;
    if (lower_left.y > other.lower_left.y)
      lower_left.y = other.lower_left.y;
    if (upper_right.x < other.upper_right.x)
      upper_right.x = other.upper_right.x;
    if (upper_right.y < other.upper_right.y)
      upper_right.y = other.upper_right.y;
    printf("OB final:"); show();
  }
  void show() const {
#if VERBOSE
    printf("LL: %3.3f, %3.3f   UR: %3.3f, %3.3f\n",
      lower_left.x, lower_left.y, upper_right.x, upper_right.y);
#endif
  }
};

}

#endif
