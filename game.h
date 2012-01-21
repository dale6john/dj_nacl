#ifndef __game___h__
#define __game___h__

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
#include "player.h"

//extern const uint32_t numbers[];
extern const uint32_t ascii[];

namespace dj {


  class GameState {
   public:
    GameState(device_t x, device_t y, double scale);
    ~GameState() {
      /*
      std::vector<Player*>::iterator it = m_players.begin();
      for ( ; it != m_players.end(); ++it) 
        delete *it;
      */
    }

   public:
    typedef enum { C_MAIN = 1, C_SOUND, C_TEXT } canvas_id_t;

   public:
    // FIXME: does this really belong in dj_inst.h ??
    void InitSimulation() {
      pthread_mutex_init(&is_on, NULL);
      pthread_mutex_init(&waiter, NULL);
      pthread_mutex_lock(&waiter);
      m_turns = 0;
    }
    void addTurns(uint32_t turns) {
      pthread_mutex_lock(&is_on);
      m_turns += turns;
      pthread_mutex_unlock(&is_on);
      if (turns > 0) {
        pthread_mutex_unlock(&waiter);
      }
    }
    bool getTurn() {
      bool ret = false;
      while (!ret) {
        if (m_turns > 0) {
          pthread_mutex_lock(&is_on);
          if (m_turns > 0) {
            m_turns--;
            ret = true;
          }
          pthread_mutex_unlock(&is_on);
        }
        if (!ret) {
          pthread_mutex_lock(&waiter);
        }
      }
      return ret;
    }
   private:
    pthread_mutex_t is_on;
    pthread_mutex_t waiter;
    uint32_t m_turns;

   public:
    void drag(device_t x, device_t y, int32_t dx, int32_t dy);
    void zoom(device_t x, device_t y, double scale = 2.0);
    void click(device_t x, device_t y);
    void quiet();
    void key(int k);
    void getCenter(double& x, double& y);
    void step();
    inline void set(uint32_t* pixel_bits, device_t x, device_t y, uint32_t color) {
      // uint32_t pys_x = m_sz_x * m_scale;
      device_t pys_x = m_sz_x;
      pixel_bits[pys_x * y + x] = color;
    }
    void setColor(uint32_t v, uint32_t c) { m_colors[v] = c; }
    void redraw(uint32_t* pixel_bits);
    View& view() { return m_view; }
    void sample(int32_t f) { 
      m_sample_frame_count = f; // use this instead of 2048
      initSoundBuffer();
    }
    void debug(char *s) {
      strcpy(m_debug, s);
    }
    void debug2(char *s) {
      strcpy(m_debug2, s);
    }

    // mixing sound buffer
    //
    // 2048 frames per channel -- 44kHz sample rate
    // 46ms of sound, 21 buffers per second
    // 10KHz highest audible pitch (15 to 20 with younger people)
    // 20 Hz lowest audible pitch
    // a wavelength should be between 4 and 2048 samples
    // upper encoding limit is 512 serial events

    //
    // 2048 frames per channel -- 44kHz sample rate
    // 46ms of sound, 21 buffers per second
    // 10KHz highest audible pitch (15 to 20 with younger people)
    // 20 Hz lowest audible pitch
    // a wavelength should be between 4 and 2048 samples
    // upper encoding limit is 512 serial events

    inline void initSoundBuffer() {
      buffer_ix = 0;
      m_music_sample_ix = 0;
      out_buffer_ix = 0;
      for (uint32_t i = 0; i < sizeof(sound_buffer)/sizeof(double); i++)
        sound_buffer[i] = 0.0;
      for (uint32_t i = 0; i < sizeof(out_sound_buffer)/sizeof(int16_t); i++)
        out_sound_buffer[i] = 0;
    }
    inline void setSoundSample(uint32_t ix, double value) {
      sound_buffer[(buffer_ix + ix) % 44100] += value; 
    }
    inline void clearBuffer(uint32_t ix, double value) {
      sound_buffer[(buffer_ix + ix) % 44100] = value;
    }
    inline double getBuffer(uint32_t ix) {
      return sound_buffer[(buffer_ix + ix) % 44100];
    }
    inline void advanceBuffer(uint32_t ix) {
      buffer_ix += ix;
      if (buffer_ix >= 44100)
        buffer_ix -= 44100;
    }

    inline void addPlayedBuffer(int16_t v) {
      out_sound_buffer[out_buffer_ix % 44100] = v;
      out_buffer_ix++;
      if (out_buffer_ix > 44100)
        out_buffer_ix = out_buffer_ix % 44100;
    }
    inline int16_t getPlayedBuffer(uint32_t ix) {
      return out_sound_buffer[(out_buffer_ix + 44100 - ix) % 44100];
    }

    // private:
    double sound_buffer[44100];
    uint32_t buffer_ix;
    uint32_t m_music_sample_ix;

    // already played sound buffer
    int16_t out_sound_buffer[44100];
    uint32_t out_buffer_ix;

   private:
    device_t m_sz_x;
    device_t m_sz_y;
    double m_scale;
    uint32_t m_seed;

    uint32_t m_colors[256];

    std::vector<Player*> m_players;

    CanvasSet m_canvas_set;
    Canvas m_canvas;
    Canvas m_sound_canvas;
    Canvas m_text_canvas;
    View m_view;
    Ascii m_ascii;
    DisplayContext m_display;

    uint32_t m_step;
    int32_t m_sample_frame_count;

    char m_debug[10240];
    char m_debug2[10240];

   public:
    bool m_sound;
    int32_t m_bang;
    int32_t m_thuds;
    bool isQuiet() { return m_quiet; }
    bool m_quiet;

    uint32_t m_boxes;
    void clearOutcomes();
    uint32_t m_outcome_ix;
    PlayAffect* m_outcomes; // FIXME: memory leak

  };

}

#endif
