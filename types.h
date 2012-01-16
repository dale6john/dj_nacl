#ifndef __types_h__
#define __types_h__

//#define VERBOSE 0
#define VVERBOSE 0
//#define PARANOID 1

#include <stdint.h>

namespace dj {

  typedef uint32_t device_t;
  const float g_version = 0.128;

  inline uint32_t fix_color(uint32_t c) {
    return ((c * 2) & 0xfefefe) | (c & 0xff000000);
    //return (c & 0x7fffffff);
  }

}

#endif
