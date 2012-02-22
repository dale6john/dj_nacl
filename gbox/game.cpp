/// @file dj_one.cc
///

#include <cstdio>
#include <string>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <cmath>

#include "player.h"
#include "game.h"

#include "ascii.h"

namespace dj {

  GameState::GameState(device_t x, device_t y, double scale)
        : m_sz_x(x), m_sz_y(y), m_scale(scale), m_seed(1),
          m_canvas_set(3),
          m_canvas(NULL, x, y),
          m_sound_canvas(NULL, x, y),
          m_text_canvas(NULL, x, y),
          m_view(m_canvas, scale),
          //m_numbers(numbers, 12, 16),
          m_ascii(ascii, 10, 20),
          m_display(&m_view, &m_ascii),
          m_selected(NULL),
          m_editor_on(false),
          m_step(0), m_sound(false),
          m_bang(0), m_thuds(0), m_quiet(false),
          m_outcomes(NULL)
  {
    init(1000); // create box elements
    m_music_sample_ix = 0;
    m_debug[0] = '\0';
    //for (uint32_t i = 0; i < m_boxes * 100; i++) 
    //  m_outcomes[i] = NONE;

    m_canvas.usable(0, 200, m_sz_x - 150, m_sz_y); // main canvas
    m_sound_canvas.usable(m_sz_x - 150, 0, m_sz_x, m_sz_y); // for clipping
    m_text_canvas.usable(0, 0, m_sz_x - 150, 200);
    m_canvas_set.add(m_canvas, C_MAIN);
    m_canvas_set.add(m_sound_canvas, C_SOUND);
    m_canvas_set.add(m_text_canvas, C_TEXT);

    m_view.center(0,0,1.0); // logical units
    //m_view.center(0,0); // logical units

    InitSimulation();
  }

  void GameState::init(uint32_t boxes) {
    srand48(1);
    m_players.clear();
    m_boxes = boxes; // up to 15k
    if (m_outcomes) {
      delete m_outcomes;
    }
    m_outcomes = new PlayAffect[m_boxes * 100];
    m_outcome_ix = 0;
    Box *box = new Box(150, 100, 30, 10);
    box->m_rect.rotate_to(0);
    box->m_rect.color(0xff00007f);
    m_players.push_back(box);

    box = new Box(300, 120, 30, 10);
    box->m_rect.rotate_to(0);
    box->m_rect.color(0xff007f00);
    m_players.push_back(box);
  }

  void GameState::redraw(uint32_t* pixel_bits) {
    // clear the canvas
    memset(pixel_bits, 0xff, m_sz_x * m_sz_y * sizeof(uint32_t));
    m_canvas_set.setPixelBits(pixel_bits);

    const uint32_t red   = 0xffff0000;
    const uint32_t green = 0xff00ff00;
    const uint32_t blue  = 0xff0000ff;
    const uint32_t ltgrey = 0xff999999;

    // borders
    m_sound_canvas.border(2, green);
    m_text_canvas.border(2, red);
    m_canvas.border(1, blue);
    // axis
    m_view.draw_axis();

    std::vector<Player*>::iterator it;
    it = m_players.begin();
    for ( ; it != m_players.end(); ++it) {
      Player *p = *it;
      p->draw(m_canvas, m_display, false);
    }

#if 1
    // frame it
    // FIXME: use canvas frame()
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

    char buf[1280];
    sprintf(buf, "v%s thuds=%d steps=%d dec=%6d sound sample=%5d oc-inq=%2dk (%s)",
      &g_version.c_str()[5],
      m_thuds, m_step,
      m_music_sample_ix,
      getPlayedBuffer(1),
      m_outcome_ix / 1000,
      m_debug
      );
#if VERBOSE
    printf("%s\n", buf);
#endif

#if 0
    // draw the sound graph
    // FIXME: factor this
    for (uint32_t y = 0; y < m_sz_y; y++) {
      int16_t v2 = getPlayedBuffer(y * 2) + getPlayedBuffer(y * 2 + 1);
      double v = double(v2) / 32676.0; // -1.0 to 1.0
      uint32_t center = 75;
      if (v > 0) {
        m_sound_canvas.hline(center, center + v * 75, y, 0xffff0000);
      } else {
        m_sound_canvas.hline(center + v * 75, center, y, 0xffff0000);
      }
    }
#endif

#if 1
    // fill the text area
    m_display.m_ascii->draw_s(m_text_canvas, buf, 10, 10, 0xffffffff);
    m_display.m_ascii->draw_s(m_text_canvas, m_view.debug(), 495, 170, 0xffffffff);

    sprintf(m_debug2, "clips: %d", m_canvas.clips());
    m_display.m_ascii->draw_s(m_text_canvas, m_debug2, 20, 170, 0xffffffff);
    for (uint32_t ix = 0; ix < theLog.bufs(); ix++) {
      if (ix < 6)
        m_display.m_ascii->draw_s(m_text_canvas, theLog.buf(ix), 25, 145 - 20 * ix, 0xffffffff);
      //else
      //  m_display.m_ascii->draw_s(m_text_canvas, theLog.buf(ix), 500, 145 - 20 * (ix - 6), 0xffffffff);
    }
#endif

    // draw the "editor"
    int16_t x = m_sound_canvas.llx() + 10;
    int16_t y = m_sound_canvas.ury() - 30;

    // bad FIXME:
    if (m_selected_program) {
      const char *s = m_selected_program->display_text();
      uint32_t color = 0xffffffff;
      m_display.m_ascii->draw_s2(m_sound_canvas, s, x, y, color);
    } else {
      uint32_t color = 0xffffffff;
      m_display.m_ascii->draw_s2(m_sound_canvas, "Select Item", x, y, color);
    }
  }

  void GameState::drag(device_t x, device_t y, int32_t dx, int32_t dy) {
    //theLog.info("drag: %d,%d  d=%d,%d", x, y, dx, dy);
    m_view.move(dx, dy);
  }

  void GameState::zoom(device_t x, device_t y, double scale) {
    int32_t tx = 0;
    int32_t ty = 0;
    /* int32_t id = */ m_canvas_set.getCanvasId(x, y, tx, ty);
    //theLog.info("gs/zoom: %d,%d  => canvas: %d  %d,%d", x, y, id, tx, ty);
    m_view.zoom(tx, ty, scale);
  }

  void GameState::key(int k, uint8_t sca, const char *dc, const char *cc, const char *co) {
    //theLog.info("KEY  %d/%c  sca:%d  ed:%d cur:%d '%s'", 
    //    k, k, sca, m_editor_on, m_editor_cursor, m_editor.c_str());
    if (k == 27) {
      // escape to "run"
      // for each player, do run, but for now, just try selected
      if (m_selected_program)
        m_selected_program->interpret();
    }
    if (m_editor_on && m_selected_program) {
      if (m_selected_program->editKey(k, sca, dc, cc, co)) {
        // bell
        theLog.info("BELL!!!");
      }
    }
  }
  void GameState::click(device_t x, device_t y) {
    int32_t tx = 0;
    int32_t ty = 0;
    int32_t id = m_canvas_set.getCanvasId(x, y, tx, ty);
    theLog.info("getCanvasId(%d %d) T(%d,%d) = %d", x, y, tx, ty, id);
    double click_x = m_view.cv2lu_x(tx);
    double click_y = m_view.cv2lu_y(ty);
    if (id == 1) {
      theLog.info("click: %3.3f,%3.3f", click_x, click_y);
      // pause the game

      // find the closed guy, and select him
      std::vector<Player*>::iterator it;
      it = m_players.begin();
      for ( ; it != m_players.end(); ++it) {
        Player *p = *it;
        double x = p->x();
        double y = p->y();
        double dx = x - click_x;
        double dy = y - click_y;
        double dist = sqrt(dx * dx + dy * dy);

        theLog.info("dist=%3.3f", dist);

        if (dist < 30) {
          if (m_selected == p) {
            m_selected = NULL;
            p->selected(false);
          } else if (m_selected) {
            m_selected->selected(false);
            p->selected(true);
            m_selected = p;
          } else {
            p->selected(true);
            m_selected = p;
          }
          if (m_selected) {
            m_editor_on = true;
            m_selected_program = p->program();
          } else {
            m_editor_on = false;
          }
          //p->click();
          break;
        }
      }
    } else if (id == 2) {
      // this is an editor click
      if (m_selected_program) {
        int32_t xx = tx - 10; // system/font constants margin
        int32_t yy = (m_sound_canvas.ury() - m_sound_canvas.lly()) - 10 - ty;
        int32_t dx = 10; // FIXME: system/font constants  width
        int32_t dy = 20; // FIXME: system/font constants  height
        double x2 = double(xx) / double(dx);
        double y2 = double(yy) / double(dy);
        theLog.info("xx=%d yy=%d (%d/%d)  (%3.3f, %3.3f)", xx, yy, dx, dy, x2, y2);
        if (x2 < 0.0) x2 = 0.0;
        if (y2 < 0.0) y2 = 0.0;
        m_selected_program->click(uint32_t(floor(y2)), uint32_t(floor(x2)));
      }
    }
  }
  void GameState::quiet() {
    m_quiet = !m_quiet;
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
