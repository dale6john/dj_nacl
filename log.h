#ifndef __dj_log_h__
#define __dj_log_h__

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

namespace dj {

  class Log {
   public:
    static const uint32_t BUFCOUNT = 12;
    static const uint32_t BUFSZ = 200;
    Log() {
      for (uint32_t i = 0; i < BUFCOUNT; i++) {
        m_bufPtr[i] = new char [BUFSZ];
      }
      m_used = 0;
      info("Init");
    }
    void info(const char *format, ...) {
      va_list ap;
      va_start(ap, format);
      info(format, ap);
    }
    void info(const char *format, va_list &ap) {
      writeMessage(" INFO  ", NULL, format,ap);
    }
    void writeMessage(const char* severity, const char *prefix, const char *format, va_list &ap) {
      vsnprintf(m_buf, sizeof(m_buf), format, ap);
#if VERBOSE
      printf("LOG: %s\n", m_buf);
#endif
      //for (int32_t i = m_used - 1; i > 0; i--) {
      //  strcpy(m_bufPtr[i], m_bufPtr[i - 1]); // TODO: not great
      //}
      // scroll
      if (m_used == BUFCOUNT) {
        for (uint32_t i = 0; i < m_used - 1; i++) {
          strncpy(m_bufPtr[i], m_bufPtr[i + 1], BUFSZ - 1); // TODO: not great
        }
      }
      if (m_used < BUFCOUNT)
        m_used++;
      strncpy(m_bufPtr[m_used - 1], m_buf, BUFSZ - 1);
    }
    uint32_t bufs() { return m_used; }
    const char* buf(uint32_t ix) { return m_bufPtr[ix]; }
   private:
    char m_buf[1024];
    char *m_bufPtr[BUFCOUNT];
    uint32_t m_used;
  };

  extern Log theLog;
}

#endif
