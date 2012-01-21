#ifndef __player_h__
#define __player_h__

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

#include "bb.h"
#include "canvas.h"
#include "view.h"
#include "point.h"
#include "types.h"
#include "gob.h"
#include "grp.h"
#include "bitmap.h"
#include "rectangle.h"

#include "display.h"

//extern const uint32_t numbers[];
extern const uint32_t ascii[];

namespace dj {

#if 0
  class Circle : public Gob {
   public:
    Circle(Point origin, Point pivot, Point reference, double radius, double angle)
        : Gob(origin, pivot, reference, angle, 1.0),
          m_radius(radius) {}
    virtual void show() {
      fix();
      char buf1[120], buf2[120], buf3[120];
      printf("circle    O%s O2%s P%s hdg:%3.3f s:%3.3f r:%3.3f\n",
        m_origin.show(buf1), m_rotated_origin.show(buf2), m_pivot.show(buf3), m_heading, m_scale, m_radius
      );
    }
    void fixbb() {
      // fix current origin (m_rotated_origin)
      if (!m_group_member) {
        m_rotated_origin.assign(m_origin);
        m_rotated_origin.add(m_reference);
        m_rotated_origin.rotate_about(m_heading, m_scale, m_pivot);
      }

      // establish bounding box
      m_box.lower_left.x  = m_rotated_origin.x - m_radius * m_scale;
      m_box.lower_left.y  = m_rotated_origin.y - m_radius * m_scale;
      m_box.upper_right.x = m_rotated_origin.x + m_radius * m_scale;
      m_box.upper_right.y = m_rotated_origin.y + m_radius * m_scale;
      m_needs_redraw = false;
    }
    virtual bool xrange(double y, double &x0, double &x1) {
      if (m_needs_redraw) fixbb();
      if (y > m_box.lower_left.y && y < m_box.upper_right.y) {
        // TODO: pythagorus may be faster
        double radius = m_radius * m_scale;
        double dy = fabs(y - m_rotated_origin.y);
        if (dy >= radius)
          return false;
        double theta = asin(dy / radius);
        /*
        if (__fpclassifyd(theta) == FP_NAN) {
          printf("y=%3.3f dy=%3.3f radius=%3.3f\n", y, dy, radius);
          return false;
        }
        */
        double cosv = cos(theta) * radius;
        x0 = m_rotated_origin.x - cosv;
        x1 = m_rotated_origin.x + cosv;
        return true;
      }
      return false;
    }

   private:
    double m_radius;
  };
#endif

  typedef enum { 
    NONE = 0x0,
    THUD = 0x1,
    BOUNCE_GROUND = 0x2,
    BOUNCE_WALL = 0x4,
    FLY = 0x8,
    PUSH = 0x10 
  } play_t;

  class PlayAffect {
   public:
    play_t m_code;
    double m_speed;
    Point m_at; // this is logical units
  };



  class Player {
   public:
    Player() : m_x(0), m_y(0), m_step(0) {}
    Player(const uint32_t *m, double x_, double y_, double h_, double sc_, uint32_t st_)
            : m_x(x_), m_y(y_), m_h(h_ / 360.0 * 2.0 * 3.14159),
              m_scale(sc_), m_step(0), m_steps(st_) {
    }
    virtual ~Player() {}

   public:
    virtual void draw(Canvas & canvas, DisplayContext & disp, bool debug) {
      // don't want to get here
      canvas.set(2,2, 0xffffff1f);
      canvas.set(2,3, 0xffffff1f);
      canvas.set(3,2, 0xffffff1f);
      canvas.set(3,3, 0xffffff1f);
      assert(!"wrong draw");
    }
    virtual void step(PlayAffect &ret) {
      if (++m_step >= m_steps)
        m_step = 0;
      ret.m_code = NONE;
      ret.m_at.assign(m_x, m_y);
    }
    virtual void click() {}

   protected:
    double m_x; // TODO: if this is a group, then we can have location,
    double m_y; //  pivot, and scale.  othewise, just steps
    double m_h; // heading
    double m_scale; // muliplier for bitmap to logical co-ordinate translation
    uint32_t m_step;
    uint32_t m_steps;
  };

  class Box : public Player {
   public:
    Box(double x, double y, double w, double h) 
        : m_speed(0), m_rotation(0.0), m_at(x, y), m_rect(m_at, w, h), angle(0.0) {
    }
    virtual void draw(Canvas & canvas, DisplayContext & disp, bool debug) {
      disp.m_view->draw(m_rect);
      /*
      for (uint32_t a = 0; a < 20; a++) {
        for (uint32_t b = 0; b < 20; b++) {
          canvas.set(a, b, 0xffffff1f);
        }
      }
      char buf[1280];
      sprintf(buf, "x=%3.3f y=%3.3f", m_at.x, m_at.y);
      m_rect.show(buf);
      buf[78] = '\0';
      disp.m_ascii->draw_s(canvas, buf, 50, 3, 0xffffffff);
      */
    }
    virtual void step(PlayAffect & ret) {
      ret.m_code = NONE;
      if (m_rotation != 0.0) {
        // active cell, spin
        m_rect.rotate_to(angle);
        angle += m_rotation;

        // apply wind and aerodynamic resistance
        Point wind(-2.5, 0);
        Point wind_d(m_speed);
        wind_d.subtract(wind);
        double force = pow(wind_d.distance(), 2.0);
        Point fv(wind_d);
        fv.scale(-1.0);
        fv.unit();
        fv.scale(force / 300.0);
        fv.add(0, -0.1); // gravity
        m_speed.add(fv);
        //ret = FLY | (static_cast<int16_t>(force) << 16);
        ret.m_code = FLY;
        ret.m_speed = m_speed.distance();

/*
        double wind_d = m_speed.x - wind;
        double force = wind_d * wind_d;
        m_speed.x -= force / 300.0;
*/

      } else if (lrand48() % 12000 == 0) {
        // inactive cell, perhaps activate
        m_rotation = (drand48() - 0.5) / 10;
        m_speed.x = pow(drand48() * 3.0, 3.0);
        m_speed.y = pow(drand48() * 8.0, 3.0);
        ret.m_code = PUSH;
        ret.m_speed = m_speed.distance();
      }

      // move the box according to speed
      if (m_speed.x != 0.0 || m_speed.y != 0.0)
        m_rect.move(m_speed.x, m_speed.y);

      // stop or bounce if it hits the ground
      if (m_rect.at().y < 0.0) {
        if (lrand48() % 100 > 5) {
          m_speed.y = -m_speed.y * (1.0 - drand48() * 0.3);
          ret.m_code = BOUNCE_GROUND;
          ret.m_speed = m_speed.distance();
          m_rotation = -m_rotation;
        } else {
          m_speed.assign(0, 0);
          m_rotation = 0.0;
          ret.m_code = THUD;
          ret.m_speed = 0;
        }
        m_rect.move_to(m_rect.at().x, 0.0);
      }
      // bounce against left wall
      if (m_rect.at().x < 0.0) {
        m_speed.x = -m_speed.x * (1.0 - drand48() * 0.3);
        m_rect.move_to(0.0, m_rect.at().y);
        if (ret.m_code == NONE) {
          ret.m_code = BOUNCE_WALL;
          ret.m_speed = m_speed.distance();
        }
      }
    }
    virtual void click() {
      // this is the 'shuffle'
      Point speed(pow((drand48() - 0.4) * 8.0, 3.0), pow((drand48() - 0.3) * 6.0, 3.0));
      m_speed = speed;
      m_rotation = (drand48() - 0.5) / 10;
    }

   private:
   public:
    Point m_speed;
    double m_rotation;
    Point m_at;
    Rectangle m_rect;
    double angle;
  };

}

#endif
