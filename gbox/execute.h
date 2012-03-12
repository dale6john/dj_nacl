#ifndef __execute_h__
#define __execute_h__

#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <vector>


inline double getTime() {
  timeval tm;
  gettimeofday( &tm, NULL );
  return (double) tm.tv_sec + ( (double) tm.tv_usec ) / 1000000.0;
}

class Yield {
 public:
  Yield() : m_reason(0), m_text("") {}
  void reason(uint32_t r) { m_reason = r; if (!r) { m_text = ""; } }
  uint32_t reason() { return m_reason; }
  void text(std::string s) { m_text = s; }
  std::string text() { return m_text; }
 private:
  uint32_t m_v;
  uint32_t m_reason;
  std::string m_text;
};


class Exec {
 public:
  Exec(int *prog, int sz);
 public:
  void show(int at, uint32_t clk, uint32_t flg);
  void list();
  void load();
  void execute(uint32_t ticks, Yield& y);
  uint16_t clock();
  void verbose(bool b) { m_verbose = b; }

 public:
  static const uint8_t SENSE = 1;
  static const uint8_t ZERO = 2;
  static const uint8_t INCR = 3;
  static const uint8_t BREAK = 4;
  static const uint8_t DO = 6;
  static const uint8_t DOMOD = 7;
  static const uint8_t RETURN = 8;
  static const uint8_t USEFRAME = 15;
  static const uint8_t TESTMM = 16;
  static const uint8_t TESTCM = 17;
  static const uint8_t TESTMC = 18;
  static const uint8_t JUMP = 32;
  static const uint8_t JUMPEQ = 33;
  static const uint8_t JUMPNE = 34;
  static const uint8_t NOP = 120;
  static const uint8_t SET = 121;
  static const uint8_t PUSH = 130;
  static const uint8_t POP = 131;

  static const uint16_t FLAGEQ = 0x001;
  static const uint16_t FLAGGT = 0x002;
  static const uint16_t FLAGLT = 0x004;
  static const uint16_t FLAGHALT = 0x008; 
  static const uint16_t FLAGFRAME = 0x010;  // enable frame interrupts
  static const uint16_t FLAGSUPER = 0x020;

  static const uint16_t TURN = 1;
  static const uint16_t MOVE = 2;
  static const uint16_t FIGHT = 3;
  static const uint16_t EAT = 4;
  static const uint16_t EXIT = 5;
  static const uint16_t COLOR = 6;
  static const uint16_t SAY = 7;

  static const uint8_t LEFT = 1;
  static const uint8_t RIGHT = 2;

 private:
  static char opnames[][12];

  static char regs[][15];

 private:
  bool m_verbose;
  int *m_prog;
  uint32_t m_sz;
};


class Location {
 public:
 private:
  float x;
  float y;
  float hdg;
};

class Ant {
 public:
  Ant(int * program, int sz, Location loc) {
    m_e = new Exec(program, sz);
    m_loc = loc;
  }
  void step() {
    Yield y;
    m_e->execute(700, y);
  }
 private:
  Exec *m_e;
  Location m_loc;
};

class Arena {
 public:
  Arena() : m_clock(0) {}
  ~Arena() {}

  void add(Exec* e) {
    m_ex.push_back(e);
  }

  int step() {
    // run an iteration.  a predetermined number of cycles for each ant.
    // first global IO sweep  - perhaps just cache neighbors
    // second execute ants
    // third apply movement / actions
    // process energy/health births/deaths

    // location

    // do we need IO refresh on ant movements?
    std::vector<Exec*>::iterator it = m_ex.begin();
    for ( ; it != m_ex.end(); ++it) {
      Exec *e = *it;
      if (e->clock() < m_clock) {
        Yield y;
        e->execute(700, y);
        if ((y.reason() & 0xff) == Exec::MOVE) {
          printf("m");
        } else if ((y.reason() & 0xff) == Exec::TURN) {
          printf("t");
        } else if (y.reason() == Exec::SAY) {
          printf("\"%s\"", y.text().c_str());
        }
#if 0
        printf("i=%d %d %d %s\n",
          (y.reason() & 0xff00) >> 8, 
          y.reason() & 0xff, e->clock(), y.text().c_str());
#endif
        if (!y.reason()) {
          // delete item from list
          return 0;
        }
      } else {
        printf(".");
      }
    }
    m_clock += 10;
    return 1;
  }


 private:
  // bunch of Exec, knows their location, heading, (speed?)
  // search structure for senses
  // physical attributes, walls, water, etc.
  // dispatching
  std::vector<Exec*> m_ex;
  uint32_t m_clock;

};


#endif
