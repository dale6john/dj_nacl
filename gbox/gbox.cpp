#include "gbox.h"

#include <limits>
#include "duppy.h"
//#include "dec.10s.48.h"

namespace dj {

  void GboxInstance::HandleMessage(const pp::Var& var_message) {
    if (!var_message.is_string()) return;
    std::string message = var_message.AsString();
    pp::Var var_reply;
    if (message == kHelloString) {
      theLog.info("message: '%s'", message.c_str());
      Hello();
    } else if (message == kPaintMethodId) {
      Paint();
    } else if (message == kClockMethodId) {
      Clock();
    } else if (message == "quiet") {
      theLog.info("message: '%s'", message.c_str());
      Quiet();
    } else {
      theLog.info("other message: '%s'", message.c_str());
      Other(message);
    }
  }

  bool GboxInstance::HandleInputEvent(const pp::InputEvent &event) {
    bool ret = false; // handled?

    switch (event.GetType()) {
    case PP_INPUTEVENT_TYPE_KEYDOWN:
    case PP_INPUTEVENT_TYPE_KEYUP:
    case PP_INPUTEVENT_TYPE_CHAR:
      {
        pp::KeyboardInputEvent key_event(event);
        ret = HandleKeyboard(key_event);
      }
      break;

    case PP_INPUTEVENT_TYPE_MOUSEDOWN:
    case PP_INPUTEVENT_TYPE_MOUSEMOVE:
    case PP_INPUTEVENT_TYPE_MOUSEUP:
    case PP_INPUTEVENT_TYPE_MOUSEENTER:
    case PP_INPUTEVENT_TYPE_MOUSELEAVE:
      {
        pp::MouseInputEvent mouse_event(event);
        ret = HandleMouse(mouse_event);
      }
      break;

    case PP_INPUTEVENT_TYPE_UNDEFINED:
    case PP_INPUTEVENT_TYPE_WHEEL:
    case PP_INPUTEVENT_TYPE_RAWKEYDOWN:
    case PP_INPUTEVENT_TYPE_CONTEXTMENU:
      ret = false;

    default:
      break;
    }
    M m(this);
    sprintf(m.buf, "_Input Event: (%s a) et:%d handled:%d count:%d", kVersion, event.GetType(), ret, ++m_mc);
    return ret;
  }

  bool GboxInstance::HandleKeyboard(const pp::KeyboardInputEvent& event) {
    uint32_t key = event.GetKeyCode();
    M m(this);
    sprintf(m.buf, "+Input Keyboard: (%s a) et:%d key:%d/%c count:%d", kVersion, event.GetType(), key, key, m_mc);

    if (key == 16) { // shift
      if (event.GetType() == 7) m_shift = true;
      if (event.GetType() == 8) m_shift = false;
    }

    if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z')) {
      m_board->key(key);
      m_board->m_bang = 0;
    }

    if (key >= 'A' && key <= 'Z') return true;
    return false;
  }

  bool GboxInstance::HandleMouse(const pp::MouseInputEvent& event) {
    PP_InputEvent_MouseButton button = event.GetButton();
    int32_t clicks = event.GetClickCount();
    pp::Point pt = event.GetPosition();

    //M m(this);
    if (button != PP_INPUTEVENT_MOUSEBUTTON_NONE) {
      // button
      //sprintf(m.buf, "+Input Button: (%s a) et:%d handled:%d count:%d", event.GetType(), button, m_mc);
    } else if (clicks) {
      //sprintf(m.buf, "+Input Click: (%s a) et:%d at:%d,%d count:%d", event.GetType(), pt.x(), pt.y(), m_mc);
    } else {
      //sprintf(m.buf, "+Input OTHER: (%s a) et:%d at:%d,%d count:%d", event.GetType(), pt.x(), pt.y(), m_mc);
    }
    if (event.GetType() == PP_INPUTEVENT_TYPE_MOUSEDOWN && button == 0) {
      M n(this);
      sprintf(n.buf, "+Input MouseDown: (%s a) et:%d at:%d,%d count:%d (%p/%p) %d", kVersion, event.GetType(), pt.x(), pt.y(), m_mc, this, m_board, button);
      m_mouse_down = pt;
      m_time_at_mouse_down = m_core->GetTimeTicks();
    }
    if (event.GetType() == PP_INPUTEVENT_TYPE_MOUSEMOVE) {
      if (m_time_at_mouse_down) {
        // drag dynamics logic
        PP_TimeTicks now = m_core->GetTimeTicks();
        bool is_drag = false;
        double dx = pt.x() - m_mouse_down.x();
        double dy = pt.y() - m_mouse_down.y();
        if (sqrt(dx * dx + dy * dy) > 30) is_drag = true;
        if (now - m_time_at_mouse_down > 0.02) is_drag = true;
        if (is_drag) {
          Drag(m_mouse_down.x(), m_mouse_down.y(),
                    m_mouse_down.x() - pt.x(), m_mouse_down.y() - pt.y());
          m_mouse_down = pt;
        }
      }
    }
    if (event.GetType() == PP_INPUTEVENT_TYPE_MOUSEUP && button == 2) {
      if (m_shift)
        ZoomIn(pt.x(), pt.y());
      else
        ZoomOut(pt.x(), pt.y());
    }
    if (event.GetType() == PP_INPUTEVENT_TYPE_MOUSEUP && button == 0) {
#if 0
      PP_TimeTicks now = m_core->GetTimeTicks();
      bool is_drag = false;
      double dx = pt.x() - m_mouse_down.x();
      double dy = pt.y() - m_mouse_down.y();
      if (sqrt(dx * dx + dy * dy) > 30) is_drag = true;
      if (now - m_time_at_mouse_down > 0.5) is_drag = true;
      if (is_drag) {
          Drag(m_mouse_down.x(), m_mouse_down.y(),
                    pt.x() - m_mouse_down.x(), pt.y() - m_mouse_down.y());
          if (0) {
            M n(this);
            sprintf(n.buf, "+Input MouseUp: DRAG (%s a) et:%d at:%d,%d count:%d (%p/%p)  %3.3f time ticks", kVersion, event.GetType(), pt.x(), pt.y(), m_mc, this, m_board, now - m_time_at_mouse_down);
          }
      } else {
        Click(m_mouse_down.x(), m_mouse_down.y());
      }
      //m_board->click(pt.x(), pt.y());
#endif
      m_time_at_mouse_down = 0;
    }
    return true;
  }



  void GboxInstance::UnlockPixels() const {
    pthread_mutex_unlock(&pixel_buffer_mutex_);
  }

  void GboxInstance::DidChangeView(const pp::Rect& position,
                                  const pp::Rect& clip) {
    if (position.size().width() == width() &&
        position.size().height() == height())
      return;  // Size didn't change, no need to update anything.

    // Create a new device context with the new size.
    DestroyContext();
    CreateContext(position.size());
    // Delete the old pixel buffer and create a new one.
    ScopedMutexLock scoped_mutex(&pixel_buffer_mutex_);
    delete pixel_buffer_;
    pixel_buffer_ = NULL;
    if (graphics_2d_context_ != NULL) {
      pixel_buffer_ = new pp::ImageData(this,
                                        PP_IMAGEDATAFORMAT_BGRA_PREMUL,
                                        graphics_2d_context_->size(),
                                        false);
    }
  }

  bool GboxInstance::Init(uint32_t argc, const char* argn[], const char* argv[]) {
    m_board = new GameState(1200, 700, 1/1.0);
    //m_board = new GameState(800, 600, 1/1.0);
    uint32_t kSampleFrameCount = 2048;
    sample_frame_count_ = pp::AudioConfig::RecommendSampleFrameCount(
          PP_AUDIOSAMPLERATE_44100, kSampleFrameCount);
    pp::AudioConfig audio_config = pp::AudioConfig(
          this, PP_AUDIOSAMPLERATE_44100, sample_frame_count_);

    assert(!audio_config.is_null());

    m_board->sample(sample_frame_count_);

    audio_ = pp::Audio(this, audio_config, SoundCallback, this);

    getBoard()->initSoundBuffer();

    this->init_sound(); // FIXME: sound cache belongs here??

    //m_board = new GameState(800, 600, 1/1.5);
    pthread_create(&compute_pi_thread_, NULL, RunBoard, this);
    return audio_.StartPlayback();
    //audio_.StartPlayback();
    //return true;
  }

  uint32_t* GboxInstance::LockPixels() {
    void* pixels = NULL;
    // Do not use a ScopedMutexLock here, since the lock needs to be held until
    // the matching UnlockPixels() call.
    if (pthread_mutex_lock(&pixel_buffer_mutex_) == kPthreadMutexSuccess) {
      if (pixel_buffer_ != NULL && !pixel_buffer_->is_null()) {
        pixels = pixel_buffer_->data();
      }
    }
    return reinterpret_cast<uint32_t*>(pixels);
  }

  void GboxInstance::CreateContext(const pp::Size& size) {
    ScopedMutexLock scoped_mutex(&pixel_buffer_mutex_);
    if (!scoped_mutex.is_valid()) {
      return;
    }
    if (IsContextValid())
      return;
    graphics_2d_context_ = new pp::Graphics2D(this, size, false);
    if (!BindGraphics(*graphics_2d_context_)) {
      printf("Couldn't bind the device context\n");
    }
  }

  void GboxInstance::DestroyContext() {
    ScopedMutexLock scoped_mutex(&pixel_buffer_mutex_);
    if (!scoped_mutex.is_valid()) {
      return;
    }
    if (!IsContextValid())
      return;
    delete graphics_2d_context_;
    graphics_2d_context_ = NULL;
  }

  void GboxInstance::Hello() {
    RequestInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD | PP_INPUTEVENT_CLASS_MOUSE);
    RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
  }

  void GboxInstance::Paint() {
    ScopedMutexLock scoped_mutex(&pixel_buffer_mutex_);
    if (!scoped_mutex.is_valid()) {
      return;
    }
    FlushPixelBuffer();

    uint32_t now = (int) time(NULL);
    if (m_lasttime != now) {
      char buf[12];
      sprintf(buf, "/%d %s", m_timecount, (now & 0x1) ? "-" : "|");
      pp::Var var_reply = pp::Var(buf);
      PostMessage(var_reply);
      m_timecount = 0;
      m_lasttime = now;
    }
    m_timecount++;
  }

  void GboxInstance::FlushPixelBuffer() {
    if (!IsContextValid())
      return;
    // Note that the pixel lock is held while the buffer is copied into the
    // device context and then flushed.
    m_board->redraw((uint32_t*)pixel_buffer_->data());
    graphics_2d_context_->PaintImageData(*pixel_buffer_, pp::Point());
    if (flush_pending())
      return;
    set_flush_pending(true);
    graphics_2d_context_->Flush(pp::CompletionCallback(&FlushCallback, this));
  }

  void GboxInstance::Clock() {
    //
    M m(this);
    m_ticks++;
    /*
    sprintf(m.buf, "+[%s] CLOCK  %d", m_text.c_str(), m_ticks);
    */
    m_board->addTurns(1);
  }

  void GboxInstance::Quiet() {
    m_board->quiet();
  }

  void GboxInstance::Other(std::string s) {
    if (s.substr(0,2) == "c/" && s.size() > 2) {
      //theLog.info("count: %s", s.c_str());
      uint32_t count = atoi(s.substr(2).c_str());
      theLog.info("count: %d", count);
      m_board->init(count);
    }
  }

  void GboxInstance::Click(uint32_t from_x, uint32_t from_y) {
    //
    M m(this);
    double x, y;
    m_board->getCenter(x, y);
    sprintf(m.buf, "+[%s] CLICK  %d  @(%f,%f)", m_text.c_str(), m_ticks, x, y);
    m_board->click(from_x, from_y);
  }

  void GboxInstance::Drag(uint32_t from_x, uint32_t from_y, int32_t dx, int32_t dy) {
    //
    double x, y;
    m_board->getCenter(x, y);
    M m(this);
    sprintf(m.buf, "+[%s] DRAG  %d  (%d,%d) @(%f,%f)", m_text.c_str(), m_ticks, dx, dy, x, y);
    m_board->drag(from_x, from_y, dx, dy);
  }

  void GboxInstance::ZoomIn(uint32_t from_x, uint32_t from_y) {
    m_board->zoom(from_x, from_y, 1.5);
    double x, y;
    m_board->getCenter(x, y);
    M m(this);
    sprintf(m.buf, "+[%s] ZOOM OUT  %d  @(%f,%f)", m_text.c_str(), m_ticks, x, y);
  }

  void GboxInstance::ZoomOut(uint32_t from_x, uint32_t from_y) {
    m_board->zoom(from_x, from_y, 1/1.5);
    double x, y;
    m_board->getCenter(x, y);
    M m(this);
    sprintf(m.buf, "+[%s] ZOOM OUT  %d  @(%f,%f)", m_text.c_str(), m_ticks, x, y);
  }

  // static
  void GboxInstance::SoundCallback(void *samples, uint32_t buffer_size, void* data) {
    GboxInstance* dj = reinterpret_cast<GboxInstance*>(data);
    //double freq = 440; // A above middle C
    int channels = 2;  // stereo
    double master_volume = 0.0;
    if (dj->getBoard()->m_sound)
      master_volume = 0.9;

    //const double delta = (M_PI * 2.0) * freq / PP_AUDIOSAMPLERATE_44100;
    const int16_t max_int16 = std::numeric_limits<int16_t>::max();
    int16_t* buff = reinterpret_cast<int16_t*>(samples);
    assert(buffer_size >= sizeof(*buff) * channels * dj->sample_frame_count_);

#if 1
    // bang noise first
    int32_t bang = dj->getBoard()->m_bang;
    if (bang == 1) {
      double bang_volume = 0.1;
      uint32_t count = 50; // samples per noise value
      double v_curr = drand48() * 2.0 - 1.0;
      double theta = 0;
      for (uint32_t ix = 0; ix < 44100 / 2; ) {
        double v_next = drand48() * 2.0 - 1.0;
        double v_slope = (v_next - v_curr) / double(count);
        //if (ix % 1000 == 0) count++;
        // use count to go from v to v_next
        for (uint32_t i = 0; i < count; i++) {
          double v = v_next;
          if (i < count / 2)
            v = v_curr + v_slope * i * 2;
          if (ix > 44100 / 20)
            bang_volume *= 0.9999;
          //buffer[(bix + ix) % 44100] += v * bang_volume + 0.0 * sin(theta);
          dj->getBoard()->setSoundSample(ix, v * bang_volume + 0.0 * sin(theta));
          ix++;
          theta += M_PI / 180 * 30;
        }
        v_curr = v_next;
      }
    }
#endif

#if 1
    // FIXME: figure out where the is_quiet boolean belongs
    if (dj->getBoard()->isQuiet()) {
      // background music/sound
      // extra '0' at the end :( so subtract 1 still use '<'
      uint32_t num_samples = sizeof(wave)/sizeof(int16_t) - 1;
      for (uint32_t j = 0; j < 2048; j++) {
        double volume = 0.2;
        if (dj->getBoard()->m_music_sample_ix < 1024 || dj->getBoard()->m_music_sample_ix > num_samples - 1044)
          volume = 0.0;
        if (dj->getBoard()->m_music_sample_ix >= num_samples)
          dj->getBoard()->m_music_sample_ix = 0;
        double volume2 = (wave[dj->getBoard()->m_music_sample_ix++]) / 32000.0;
        volume2 *= volume;
        dj->getBoard()->setSoundSample(j, volume2);
        dj->getBoard()->m_music_sample_ix++; // ignore one channel
      }
    }
#endif

    Point my_ear(20,10);

#if 0
    if (0) {
      double theta = 0;
      double delta = 30 * M_PI / 180;
      uint32_t at = 0;
      for (uint32_t j = 0; j < 2000; j++) {
        dj->getBoard()->setSoundSample(at + j, 0.3 * sin(theta));
        theta += delta;
        if (theta > 2 * M_PI) theta -= 2 * M_PI;
        delta *= 0.999;
      }
    }
#endif
#if 1
    // apply the 'outcomes'
    PlayAffect * outcomes = dj->getBoard()->m_outcomes;
    int32_t limit = 4000;
    for (uint32_t i = 0; i < dj->getBoard()->m_outcome_ix; i++) {
      play_t outcome = outcomes[i].m_code;
      double speed = outcomes[i].m_speed;
      double area = outcomes[i].m_area;
      Point outcome_at = outcomes[i].m_at;
      switch(outcome) {
        case NONE:
          break;
        case THUD:
          if (0) {
            uint32_t count = 50; // samples per noise value
            double v_curr = drand48() * 2.0 - 1.0;
            double thud_volume = 0.01;
            uint32_t at = lrand48() % 2000;
            for (uint32_t ix = 0; ix < 4000; ) {
              double v_next = drand48() * 2.0 - 1.0;
              double v_slope = (v_next - v_curr) / double(count);
              //if (ix % 1000 == 0) count++;
              // use count to go from v to v_next
              double ramp = 0.0;
              for (uint32_t i = 0; i < count; i++) {
                if (i < 100) ramp += 0.01;
                double v = v_next;
                if (i < count / 2)
                  v = v_curr + v_slope * i * 2;
                if (ix > 44100 / 40)
                  thud_volume *= 0.9999;
                dj->getBoard()->setSoundSample(at + ix, v * ramp * thud_volume);
                ix++;
              }
              v_curr = v_next;
            }
          }
          break;
        case BOUNCE_GROUND:
          {
            // from 10kHz to 1kHz
            // 4 samples to 40 samples
            if (speed > 0.5) {
              if (limit >= 0) {
                limit--;
                dj->add_sound(outcome, speed, area, dj->getBoard());
              }
            }
          }
          break;
        case BOUNCE_GROUND2:
          {
            // from 10kHz to 1kHz
            // 4 samples to 40 samples
            //if (speed > 0.5) {
            //  if (limit >= 0) {
                limit--;
                dj->add_sound(outcome, speed, area, dj->getBoard());
            //  }
            //}
          }
          break;
        case BOUNCE_WALL:
          if (0) {
            // from 10kHz to 1kHz
            // 4 samples to 40 samples
            double theta = 0;
            double delta = 20 * M_PI / 180;
            uint32_t at = lrand48() % 2000;
            // FIXME: this is too slow
            for (uint32_t j = 0; j < 2000; j++) {
              dj->getBoard()->setSoundSample(at + j, 0.01 * sin(theta));
              theta += delta;
              if (theta > 2 * M_PI) theta -= 2 * M_PI;
              delta *= 0.999;
            }
          }
          break;
        case FLY:
          // continual sound -- freq for duration
          if (0) {
            // from 10kHz to 1kHz
            // 4 samples to 40 samples
            double theta = 0;
            double delta = 30 * M_PI / 180;
            uint32_t at = lrand48() % 2000;
            for (uint32_t j = 0; j < 2000; j++) {
              dj->getBoard()->setSoundSample(at + j, 0.2 * sin(theta));
              theta += delta;
              if (theta > 2 * M_PI) theta -= 2 * M_PI;
              delta *= 0.999;
            }
          }
          break;
        case PUSH:
          // little, spontaneous bounce
          if (0) {
            uint32_t count = 50; // samples per noise value
            double v_curr = drand48() * 2.0 - 1.0;
            double push_volume = 0.02;
            uint32_t at = lrand48() % 2000;
            for (uint32_t ix = 0; ix < 4000; ) {
              double v_next = drand48() * 2.0 - 1.0;
              double v_slope = (v_next - v_curr) / double(count);
              //if (ix % 1000 == 0) count++;
              // use count to go from v to v_next
              for (uint32_t i = 0; i < count; i++) {
                double v = v_next;
                if (i < count / 2)
                  v = v_curr + v_slope * i * 2;
                if (ix > 44100 / 40)
                  push_volume *= 0.9999;
                dj->getBoard()->setSoundSample(at + ix, v * push_volume);
                ix++;
              }
              v_curr = v_next;
            }
          }
          break;
        default:
          break;
      }
    }
#endif
    dj->getBoard()->clearOutcomes();
    char buf[100];
    sprintf(buf, "h/m/t %d/%d/%d", dj->cache_hit, dj->cache_miss, dj->cache_tries);
    dj->getBoard()->debug(buf);

    //
    // "mix"
    //
    for (uint32_t ix = 0; ix < 2048; ix++) {
      double v = dj->getBoard()->getBuffer(ix) / 2.0;
      /*
       * supposed to be high frequency filter
      for (uint32_t i = 0; i < 20; i++) {
        v += dj->getBoard()->getBuffer(ix + i) / double(i + 2);
      }
      */
      dj->getBoard()->clearBuffer(ix, 0.0);
      int16_t sign = (v > 0) ? 1 : -1;
      v = (v > 0) ? v : -v;
      v *= master_volume;
      if (v >  0.49) v =  0.49;
      if (v < -0.40) v = -0.49;
      //double v2 = log(1 + v)/log(500) * max_int16;
      double v2 = double(v) * double(max_int16) * double(sign);
      int16_t scaled_value = static_cast<int16_t>(v2);
      *buff++ = scaled_value; // left
      *buff++ = scaled_value; // right

      dj->getBoard()->addPlayedBuffer(scaled_value);
    }
    dj->getBoard()->advanceBuffer(2048);

  }

  void* GboxInstance::RunBoard(void* param) {
    GboxInstance* dj = static_cast<GboxInstance*>(param);
    for (int i = 0; i < 1000000 && !dj->quit(); i++) {
      //if (dj->flush_pending()) continue;
      dj->getBoard()->step();
      dj->setCount(i);
      /* PP_Time t = dj->getCore()->GetTime(); */
#if 0
      uint32_t z = 0;
      for (uint32_t ix = 0; ix < slowingLoop; ix++)
        z ^= ix;
#endif
      char buf[120];
#if 0
// 68.87.76.182, 78.134
      double t = 0;
        /* PP_TimeTicks */ t = dj->getCore()->GetTimeTicks();
#endif
      sprintf(buf, "i=%d (v:%3.2f), flush=%d ",
          i, version, dj->flush_pending());
      dj->setText(buf);
      dj->getBoard()->getTurn();

    }
    return 0;
  }

}


/// The Module class.  The browser calls the CreateInstance() method to create
/// an instance of your NaCl module on the web page.  The browser creates a new
/// instance for each <embed> tag with type="application/x-nacl".
class GboxModule : public pp::Module {
 public:
  GboxModule() : pp::Module(), count(0) {}
  virtual ~GboxModule() {}

  /// Create and return a GboxInstance object.
  /// @param[in] instance The browser-side instance.
  /// @return the plugin-side instance.
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    dj::GboxInstance * dj = new dj::GboxInstance(instance);
    dj->setCore(core());
#if 0
    PP_Time t = core()->GetTime();
    char buf[120];
    sprintf(buf, "time=%3.6f", t);
    dj->setText(buf);
#endif
    return dj;
  }
 private:
  uint32_t count;
};



namespace pp {
  /// Factory function called by the browser when the module is first loaded.
  /// The browser keeps a singleton of this module.  It calls the
  /// CreateInstance() method on the object you return to make instances.  There
  /// is one instance per <embed> tag on the page.  This is the main binding
  /// point for your NaCl module with the browser.
  Module* CreateModule() {
    return new GboxModule();
  }
}  // namespace pp


