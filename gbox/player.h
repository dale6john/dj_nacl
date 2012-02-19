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
#include "circle.h"

#include "display.h"

//extern const uint32_t numbers[];
extern const uint32_t ascii[];

namespace dj {

  typedef enum { 
    NONE = 0x0,
    THUD = 0x1,
    BOUNCE_GROUND = 0x2,
    BOUNCE_GROUND2 = 0x4,
    BOUNCE_WALL = 0x8,
    FLY = 0x10,
    PUSH = 0x20 
  } play_t;

  class PlayAffect {
   public:
    play_t m_code;
    double m_speed;
    double m_area;
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
        ret.m_area = m_rect.area();

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

  class PCircle : public Player {
   public:
    PCircle(double x, double y, double r)
        : m_speed(0), m_rotation(0.0), m_at(x, y), m_circle(m_at, r), angle(0.0) {
    }
    virtual void draw(Canvas & canvas, DisplayContext & disp, bool debug) {
      disp.m_view->draw(m_circle);
    }
    virtual void step(PlayAffect & ret) {
      ret.m_code = NONE;
      if (m_rotation != 0.0) {
        // active cell, spin
        m_circle.rotate_to(angle);
        angle += m_rotation;

        // apply wind and aerodynamic resistance
        Point wind(-2.5, 0);
        Point wind_d(m_speed);
        wind_d.subtract(wind);
        double force = pow(wind_d.distance(), 1.6);
        Point fv(wind_d);
        fv.scale(-1.0);
        fv.unit();
        fv.scale(force / 300.0);
        fv.add(0, -0.1); // gravity
        m_speed.add(fv);
        //ret = FLY | (static_cast<int16_t>(force) << 16);
        ret.m_code = FLY;
        ret.m_speed = m_speed.distance();
        ret.m_area = m_circle.area();

/*
        double wind_d = m_speed.x - wind;
        double force = wind_d * wind_d;
        m_speed.x -= force / 300.0;
*/

      } else if (lrand48() % 4000 == 0) {
        // inactive cell, perhaps activate
        m_rotation = (drand48() - 0.5) / 10;
        m_speed.x = pow(drand48() * 3.0, 3.0);
        m_speed.y = pow(drand48() * 8.0, 3.0);
        ret.m_code = PUSH;
        ret.m_speed = m_speed.distance();
      }

      // move the box according to speed
      if (m_speed.x != 0.0 || m_speed.y != 0.0)
        m_circle.move(m_speed.x, m_speed.y);

      // stop or bounce if it hits the ground
      if (m_circle.at().y < 0.0) {
        if (lrand48() % 100 > 2) {
          m_speed.y = -m_speed.y * (1.0 + drand48() * 0.92);
          ret.m_code = BOUNCE_GROUND2;
          ret.m_speed = m_speed.distance();
          m_rotation = -m_rotation;
        } else {
          m_speed.assign(0, 0);
          m_rotation = 0.0;
          ret.m_code = THUD;
          ret.m_speed = 0;
        }
        m_circle.move_to(m_circle.at().x, 0.0);
      }
      // bounce against left wall
      if (m_circle.at().x < 0.0) {
        m_speed.x = -m_speed.x * (1.0 - drand48() * 0.3);
        m_circle.move_to(0.0, m_circle.at().y);
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
    Circle m_circle;
    double angle;
  };

}

#endif
