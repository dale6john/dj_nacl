#ifndef __dj_inst__
#define __dj_inst__

#include "dj_two.h"

#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/graphics_2d.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/cpp/size.h"
#include "ppapi/cpp/core.h"
#include "ppapi/cpp/audio.h"

namespace dj {

class DjTwoInstance : public pp::Instance {
 public:
  /// The constructor creates the plugin-side instance.
  /// @param[in] instance the handle to the browser-side plugin instance.
  explicit DjTwoInstance(PP_Instance instance) 
      : pp::Instance(instance), m_count(0), m_mc(0), 
        m_board(NULL), m_ticks(0), m_time_at_mouse_down(0),
        m_shift(false)
        {}
  virtual ~DjTwoInstance() {
    if (m_board) delete m_board;
  }

  static const double version = 0.01;

  void setCount(uint32_t i) { m_count = i; }
  void setText(std::string s) { m_text = s; }
  void setCore(pp::Core* c) { m_core = c; }

  void setBoard(GameState* b) { m_board = b; }
  GameState* getBoard() { return m_board; }

  /// Handler for messages coming in from the browser via postMessage().  The
  /// @a var_message can contain anything: a JSON string; a string that encodes
  /// method names and arguments; etc.  For example, you could use
  /// JSON.stringify in the browser to create a message that contains a method
  /// name and some parameters, something like this:
  ///   var json_message = JSON.stringify({ "myMethod" : "3.14159" });
  ///   nacl_module.postMessage(json_message);
  /// On receipt of this message in @a var_message, you could parse the JSON to
  /// retrieve the method name, match it to a function call, and then call it
  /// with the parameter.
  /// @param[in] var_message The message posted by the browser.
  virtual void HandleMessage(const pp::Var& var_message);

  virtual bool HandleInputEvent(const pp::InputEvent &event);

 private:
  bool HandleKeyboard(const pp::KeyboardInputEvent& event);
  bool HandleMouse(const pp::MouseInputEvent& event);

  // sound stuff
  static void SoundCallback(void * samples, uint32_t buffer_size, void* data);
  pp::Audio audio_;
  double theta_;
  uint32_t sample_frame_count_;

  // code to cache sounds we get
  static const uint32_t sound_cache_items = 100;
  static const uint32_t sound_cache_each = 10000; // about 250 ms - mono
  double sound_cache[sound_cache_items * sound_cache_each];
  uint32_t sound_cache_sz[sound_cache_items];
  uint32_t cache_hit;
  uint32_t cache_miss;
  uint32_t cache_tries;
  void init_sound() {
    for (uint32_t i = 0; i < sound_cache_items; i++) {
      for (uint32_t n = 0; n < sound_cache_each; n++) {
        sound_cache[i * sound_cache_each + n] = 0.0; 
      }
      sound_cache_sz[i] = 0;
    }
    cache_hit = cache_miss = cache_tries = 0;
  }
  void add_sound(play_t outcome, double speed, GameState* board) {
    if (outcome == BOUNCE_GROUND) {
      double theta = 0.0;
      double delta = 30 * M_PI / 180;
      double volume2 = log(speed)/log(5);
      uint32_t at = lrand48() % 2000;

      int32_t six = uint32_t(floor(speed * 10));
      if (six >= int32_t(sound_cache_items))
        six = sound_cache_items - 1;
      double *cache = NULL;
      bool valid_cache = true;
      bool have_cache = valid_cache && (sound_cache_sz[six] > 0);
      if (valid_cache) 
        cache = &sound_cache[six * sound_cache_each];
      cache_tries++;

      if (!have_cache) {
        cache_miss++;
        double volume = 0.0;
        for (uint32_t j = 0; j < floor(speed * 100); j++) {
          if (volume < 0.2)
            volume += 0.004;
          double sample = volume * volume2 * sin(theta);
          board->setSoundSample(at + j, sample);
          if (valid_cache) {
            cache[j] = sample;
            sound_cache_sz[six]++;
          }

          theta += delta;
          if (theta > 2 * M_PI) theta -= 2 * M_PI;
          if (delta > 15 * M_PI / 180)
            delta *= 0.999;
          if (volume2 > 0.2)
            volume2 *= 0.99;
        }
      } else {
        // apply cache
        cache_hit++;
        for (uint32_t j = 0; j < sound_cache_sz[six]; j++) {
          double sample = cache[j];
          board->setSoundSample(at + j, sample);
        }
      }
    }
  }

 public:

  // Start up the ComputePi() thread.
  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);

  // Update the graphics context to the new size, and regenerate |pixel_buffer_|
  // to fit the new size as well.
  virtual void DidChangeView(const pp::Rect& position, const pp::Rect& clip);

  // Called by the browser to handle the postMessage() call in Javascript.
  // The message in this case is expected to contain the string 'paint', and
  // if so this invokes the Paint() function.  If |var_message| is not a string
  // type, or contains something other than 'paint', this method posts an
  // invalid value for Pi (-1.0) back to the browser.
  //virtual void HandleMessage(const pp::Var& var_message);

  // Return a pointer to the pixels represented by |pixel_buffer_|.  When this
  // method returns, the underlying |pixel_buffer_| object is locked.  This
  // call must have a matching UnlockPixels() or various threading errors
  // (e.g. deadlock) will occur.
  uint32_t* LockPixels();
  // Release the image lock acquired by LockPixels().
  void UnlockPixels() const;

  // Flushes its contents of |pixel_buffer_| to the 2D graphics context.  The
  // ComputePi() thread fills in |pixel_buffer_| pixels as it computes Pi.
  // This method is called by HandleMessage when a message containing 'paint'
  // is received.  Echos the current value of pi as computed by the Monte Carlo
  // method by posting the value back to the browser.
  void Paint();
  void Clock();
  void Quiet();
  void Drag(uint32_t from_x, uint32_t from_y, int32_t dx, int32_t dy);
  void Click(uint32_t from_x, uint32_t from_y);
  void ZoomIn(uint32_t from_x, uint32_t from_y);
  void ZoomOut(uint32_t from_x, uint32_t from_y);

  bool quit() const {
    return false;
  }

  double pi() const {
    return m_count;
  }

  int width() const {
    return pixel_buffer_ ? pixel_buffer_->size().width() : 0;
  }
  int height() const {
    return pixel_buffer_ ? pixel_buffer_->size().height() : 0;
  }

  // Indicate whether a flush is pending.  This can only be called from the
  // main thread; it is not thread safe.
  bool flush_pending() const {
    return flush_pending_;
  }
  void set_flush_pending(bool flag) {
    flush_pending_ = flag;
  }

 private:
  // Create and initialize the 2D context used for drawing.
  void CreateContext(const pp::Size& size);
  // Destroy the 2D drawing context.
  void DestroyContext();
  // Push the pixels to the browser, then attempt to flush the 2D context.
  // If there is a pending flush on the 2D context, then update the pixels
  // only and do not flush.
  void FlushPixelBuffer();

  static void* RunBoard(void* param);

  static void* ComputePi(void* param);

  bool IsContextValid() const {
    return graphics_2d_context_ != NULL;
  }

  mutable pthread_mutex_t pixel_buffer_mutex_;
  pp::Graphics2D* graphics_2d_context_;
  pp::ImageData* pixel_buffer_;
  bool flush_pending_;
  bool quit_;
  pthread_t compute_pi_thread_;
  double pi_;

 private:
  uint32_t m_count;
  std::string m_text;
  uint32_t m_mc;
  GameState* m_board;
  pp::Core* m_core;
  uint32_t m_ticks;
  PP_TimeTicks m_time_at_mouse_down;
  pp::Point m_mouse_down;
  bool m_shift;
};

}

namespace {
  const char* const kHelloString = "hello";
  const char* const kReplyString = "hello from NaCl";

  const char* const kVersion = "0.19";
  const uint32_t slowingLoop = 1000000;

  const int kPthreadMutexSuccess = 0;
  const char* const kPaintMethodId = "paint";
  const char* const kClockMethodId = "clock";
  const double kInvalidPiValue = -1.0;
  const int kMaxPointCount = 1000000000;  // The total number of points to draw.
  const uint32_t kOpaqueColorMask = 0xff000000;  // Opaque pixels.
  const uint32_t kRedMask = 0xff0000;
  const uint32_t kBlueMask = 0xff;
  const uint32_t kRedShift = 16;
  const uint32_t kBlueShift = 0;

  // This is called by the browser when the 2D context has been flushed to the
  // browser window.
  void FlushCallback(void* data, int32_t result) {
    static_cast<dj::DjTwoInstance*>(data)->set_flush_pending(false);
  }
}

namespace dj {
  class M {
   public:
    M(pp::Instance *inst) : m_inst(inst) { buf[0] = '\0'; }
    ~M() {
      if (buf[0]) {
        pp::Var var_reply = pp::Var(buf);
        m_inst->PostMessage(var_reply);
      }
    }
    char buf[1024];
   private:
    pp::Instance * m_inst;
  };


  // A small helper RAII class that implementes a scoped pthread_mutex lock.
  class ScopedMutexLock {
   public:
    explicit ScopedMutexLock(pthread_mutex_t* mutex) : mutex_(mutex) {
      if (pthread_mutex_lock(mutex_) != kPthreadMutexSuccess) {
        mutex_ = NULL;
      }
    }
    ~ScopedMutexLock() {
      if (mutex_)
        pthread_mutex_unlock(mutex_);
    }
    bool is_valid() const {
      return mutex_ != NULL;
    }
   private:
    pthread_mutex_t* mutex_;  // Weak reference.
  };

  // A small helper RAII class used to acquire and release the pixel lock.
  class ScopedPixelLock {
   public:
    explicit ScopedPixelLock(DjTwoInstance* image_owner)
        : image_owner_(image_owner), pixels_(image_owner->LockPixels()) {}

    ~ScopedPixelLock() {
      pixels_ = NULL;
      image_owner_->UnlockPixels();
    }

    uint32_t* pixels() const {
      return pixels_;
    }
   private:
    DjTwoInstance* image_owner_;  // Weak reference.
    uint32_t* pixels_;  // Weak reference.

    ScopedPixelLock();  // Not implemented, do not use.
  };


}



#endif
