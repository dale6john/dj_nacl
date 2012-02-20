#ifndef __gob_h__
#define __gob_h__

#include "types.h"
#include "bb.h"
#include "point.h"

namespace dj {

/*
 * concrete storage for shared graphic drawing state
 */
class Drawable {
 public:
  Drawable(Point pivot, Point at, double scale, double angle)
    : m_color(0xffffffff), m_color2(0xffffffff), 
        m_at(at), m_pivot(pivot), m_scale(scale), m_heading(angle),
        m_needs_redraw(true), m_visible(true), m_filled(true) {}
  Drawable(Point at, double scale, double angle)
    : m_color(0xffffffff), m_color2(0xffffffff), 
        m_at(at), m_scale(scale), m_heading(angle),
        m_needs_redraw(true), m_visible(true), m_filled(true) {
    m_pivot.assign(0.0, 0.0);
  }
 private:
  Drawable(const Drawable& other) {}
 public:
  virtual ~Drawable() {}
  // steps are intrinsic to gobs ??
  virtual bool hasGroup() { return false; }
  virtual void show() { assert(!"no show\n"); }
  virtual void show(char *s) { assert(!"no show\n"); }
  virtual void fixbb() { assert(!"no fixbb\n"); }
  virtual void transform(
            double grp_heading, double grp_scale, Point& grp_pivot,
            Point& grp_ref, Point& grp_at) {}

  inline void color(uint32_t c) {
    m_color = c;
  }
  inline uint32_t color() {
    return m_color;
  }
  inline void filled(bool b) {
    m_filled = b;
  }
  inline bool filled() const { return m_filled; }

  inline void scale_to(double scale) {
    m_scale = scale;
    m_needs_redraw = true;
  }
  inline void rotate_to(double angle) {
    m_heading = angle;
    m_needs_redraw = true;
  }
  inline void rotate(double angle) {
    m_heading += angle;
    m_needs_redraw = true;
  }
  inline void move(double dx, double dy) {
    m_at.x += dx;
    m_at.y += dy;
    m_needs_redraw = true;
  }
  inline void move_to(double dx, double dy) {
    m_at.x = dx;
    m_at.y = dy;
    m_needs_redraw = true;
  }
  inline Point& at() { return m_at; }

  BoundingBox& boundingBox() { 
    // const but mutable
    fix(); 
    return m_box; 
  }

  virtual void fix() {
    // make sure we don't really fix-on-draw, but fix-on-modify
    if (m_needs_redraw)
      fixbb();
  }
  // FIXME: make iterator model
  virtual void xrange_reset() { 
    if (hasGroup()) {
      xrange_reset();
    }
    //m_xrange_ready = true; 
  }
  virtual bool xrange(double y, double &x0, double &x1) { return false; }

 protected:
  uint32_t m_color;
  uint32_t m_color2;

  BoundingBox m_box;

  Point m_at;
  Point m_pivot;
  double m_scale;
  double m_heading;

  bool m_needs_redraw;
  //bool m_group_member;
  bool m_visible;
  bool m_filled;
  //bool m_xrange_ready;
};

#if 0
bool Drawable::xrange(double y, double &x0, double &x1) {
  // EXTERNAL
  bool ret = false;
  if (hasGroup()) {
    printf("GROUP xrange\n");
    ret = xrange(y, x0, x1);
    //exit(0);
  } else {
    //if (m_xrange_ready) {
    //  ret = xrange(y, x0, x1);
    //  m_xrange_ready = false;
    //}
    return ret;
  }
}
#endif

}

#endif
