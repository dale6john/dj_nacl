/// @file dj_one.cc
/// This example demonstrates loading, running and scripting a very simple NaCl
/// module.  To load the NaCl module, the browser first looks for the
/// CreateModule() factory method (at the end of this file).  It calls
/// CreateModule() once to load the module code from your .nexe.  After the
/// .nexe code is loaded, CreateModule() is not called again.
///
/// Once the .nexe code is loaded, the browser than calls the CreateInstance()
/// method on the object returned by CreateModule().  It calls CreateInstance()
/// each time it encounters an <embed> tag that references your NaCl module.
///
/// The browser can talk to your NaCl module via the postMessage() Javascript
/// function.  When you call postMessage() on your NaCl module from the browser,
/// this becomes a call to the HandleMessage() method of your pp::Instance
/// subclass.  You can send messages back to the browser by calling the
/// PostMessage() method on your pp::Instance.  Note that these two methods
/// (postMessage() in Javascript and PostMessage() in C++) are asynchronous.
/// This means they return immediately - there is no waiting for the message
/// to be handled.  This has implications in your program design, particularly
/// when mutating property values that are exposed to both the browser and the
/// NaCl module.

#include <cstdio>
#include <string>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <cmath>
/*
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/graphics_2d.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/cpp/size.h"
#include "ppapi/cpp/core.h"
#include "ppapi/cpp/input_event.h"
*/

#include "dj_two.h"

//#include "numbers.h"
#include "ascii.h"
/*
#include "ladybug.h"
#include "flower.h"
#include "clover.h"
*/


namespace dj {

  GameState::GameState(device_t x, device_t y, double scale)
        : m_sz_x(x), m_sz_y(y), m_scale(scale), m_seed(1),
          m_view(x, y, scale),
          //m_numbers(numbers, 12, 16),
          m_ascii(ascii, 10, 20),
          m_display(&m_view, &m_ascii),
          m_step(0), m_sound(false),
          m_bang(0)
  {
    // constructed with physical dimensions and scale 1

    //Player *f  = new Flower(300, 150);
    //Player *b1 = new Ladybug(x - 50, y - 60, 15.0/360.0 * 3.14159*2.0, 0.5);
    //Player *b2 = new Ladybug(50, 80, 25, 1.0);

    //m_players.push_back(f);
    //m_players.push_back(b1);
    //m_players.push_back(b2);

    srand48(1);
    // up to 15k
    m_boxes = 300;
    for (uint32_t i = 0; i < m_boxes; i++) {
      double x = 1.0 + drand48() * 149.0;
      double y = 1.0 + drand48() * 149.0;
      double w = drand48() * 2.0 + 4.0;
      double h = drand48() * 9.0 + 4.0;

      Box *box = new Box(x + 250, y + 300, w, h);
      double scale = drand48() + 0.5;
      double angle = (drand48() * 2 - 1) * M_PI;
      //Point speed(pow(drand48() * 4.0, 3.0), pow(drand48() * 4.0, 3.0));
      Point speed(0,0);
      box->m_rect.scale_to(scale);
      box->m_rect.rotate_to(angle);
      box->m_rect.color(0xff00007f 
        + (((int)floor(x/150.0*256.0)) << 16)
        + (((int)floor(y/150.0*256.0)) << 8));
      if (true || lrand48() % 2 == 1) {
        box->m_speed = speed;
        box->m_rotation = (drand48() - 0.5) / 4.0;
      } else {
        box->m_rotation = 0;
      }
      m_players.push_back(box);
    }
    m_thuds = 0;
    m_music_sample_ix = 0;
    m_outcomes = new PlayAffect[m_boxes * 100];
    m_outcome_ix = 0;
    //for (uint32_t i = 0; i < m_boxes * 100; i++) 
    //  m_outcomes[i] = NONE;

    m_view.center(400,300);

    InitSimulation();
  }

  void GameState::redraw(uint32_t* pixel_bits) {
    std::vector<Player*>::iterator it;
    it = m_players.begin();
    //uint32_t color = m_colors[C_FIRE];
    /*
     * need to un-draw previous iteration
     */
    // FIXME: there must be a faster way to do this?
    // just memset pixel_bits
    /*
    for (uint32_t x = 0; x < m_sz_x; x++)
      for (uint32_t y = 0; y < m_sz_y; y++)
        set(pixel_bits, x, y, 0xffffffff);
    */
    // clear the canvas
    memset(pixel_bits, 0xff, m_sz_x * m_sz_y * sizeof(uint32_t));

    Canvas canvas(pixel_bits, m_sz_x, m_sz_y);
    //canvas.usable(0, m_sz_x, 0, m_sz_y - 150);
    canvas.usable(0, 0, m_sz_x - 150, m_sz_y);
    m_view.draw_axis(canvas);
    for ( ; it != m_players.end(); ++it) {
      Player *p = *it;
      p->draw(canvas, m_display, false);
    }

#if 1
    // frame it
    for (device_t x = 0; x < m_sz_x; x++) {
      set(pixel_bits, x, 0, 0xffffffff);
      set(pixel_bits, x, 1, 0xff000000);
      set(pixel_bits, x, m_sz_y - 2, 0xff000000);
      set(pixel_bits, x, m_sz_y - 1, 0xffffffff);
    }
    for (device_t y = 1; y < m_sz_y - 1; y++) {
      set(pixel_bits, 0, y, 0xffffffff);
      set(pixel_bits, 1, y, 0xff000000);
      set(pixel_bits, m_sz_x - 2, y, 0xff000000);
      set(pixel_bits, m_sz_x - 1, y, 0xffffffff);
    }
#endif
    canvas.set(2,2, 0xffffff1f);
    canvas.set(2,3, 0xffffff1f);
    canvas.set(3,2, 0xffffff1f);
    canvas.set(3,3, 0xffffff1f);

    char buf[1280];
    sprintf(buf, "v%s frames=%d thuds=%d steps=%d dec=%d sound sample=%d  outcomes-in-queue=%dk  ",
        g_version.c_str(),
        m_sample_frame_count, m_thuds, m_step,
        m_music_sample_ix,
        getPlayedBuffer(1),
        m_outcome_ix / 1000);
    m_display.m_ascii->draw_s(canvas, buf, 50, 3, 0xffffffff);

    // draw the sound graph
    // FIXME: factor this
    Canvas sound_canvas(pixel_bits, m_sz_x, m_sz_y);
    for (uint32_t y = 0; y < m_sz_y; y++) {
      //double v = getBuffer(y * 2) + getBuffer(y * 2 + 1);
      int16_t v2 = getPlayedBuffer(y * 2);// + getPlayedBuffer(y * 2 + 1);
      double v = double(v2) / 32676.0; // -1.0 to 1.0
      uint32_t center = m_sz_x - 75;;
      if (v > 0) { 
        sound_canvas.hline(center, center + v * 75, y, 0xffff0000);
      } else {
        sound_canvas.hline(center + v * 75, center, y, 0xffff0000);
      }
    }
  }

  void GameState::drag(device_t x, device_t y, int32_t dx, int32_t dy) {
    m_view.move(dx, dy);
  }

  void GameState::zoom(device_t x, device_t y, double scale) {
    m_view.zoom(x, y, scale);
  }
  void GameState::key(int k) {
    std::vector<Player*>::iterator it;
    it = m_players.begin();
    for ( ; it != m_players.end(); ++it) {
      (*it)->click();
    }
  }
  void GameState::click(device_t x, device_t y) {
    /*
    std::vector<Player*>::iterator it;
    it = m_players.begin();
    for ( ; it != m_players.end(); ++it) {
      (*it)->click();
    }
    */
  }
  void GameState::getCenter(double& x, double& y) {
    m_view.getCenter(x, y);
  }
  void GameState::clearOutcomes() {
    m_outcome_ix = 0;
  }

  void GameState::step() {
    if (m_step++ > 50) {
      m_sound = true;
      m_bang++;
      std::vector<Player*>::iterator it;
      it = m_players.begin();
      for ( ; it != m_players.end(); ++it) {
        PlayAffect ret;
        (*it)->step(ret);
        if (ret.m_code == THUD) m_thuds++;
        if (m_outcome_ix < m_boxes * 10 && ret.m_code != BOUNCE_WALL)
          m_outcomes[m_outcome_ix++] = ret;
      }
    }
  }


  /*
  inline void setBuffer(uint32_t ix, double v) { 
    buffer[(buffer_ix + ix) % 44100] += v; 
  }
  inline void clearBuffer(uint32_t ix, double v) { 
    buffer[(buffer_ix + ix) % 44100] = v; 
  }
  inline double getBuffer(uint32_t ix) {
    return buffer[(buffer_ix + ix) % 44100];
  }
  inline void advanceBuffer(uint32_t ix) {
    buffer_ix += ix;
    if (buffer_ix >= 44100)
      buffer_ix -= 44100;
  }
  */
} // namespace dj
