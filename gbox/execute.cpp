#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "execute.h"

char Exec::opnames[][12] = {
    "", "SENSE", "ZERO", "INCR", "BREAK", "", "DO", "DOMOD", "RETURN", "",
    "", "", "", "", "", "USEFRA", "TESTMM", "TESTCM", "TESTMC", "", 
    "", "", "", "", "", "", "", "", "", "",
    "", "", "JUMP", "JUMPEQ", "JUMPNE" }; 

char Exec::regs[][15] = {
    "_pc", "_flags", "_user", "_clock", "_frame", "_color", "_status", "_action",
    "_sense_food", "_sense_enemy", "_health", "_energy", "_pc2", "_flags2" };

Exec::Exec(int *prog, int sz) : m_verbose(false) {
  //m_prog = prog;
  m_prog = new int [sz + 1];
  memcpy((char *)m_prog, (char *)prog, sz * sizeof(int));
  m_sz = sz;
  load();
}

void Exec::show(int at, uint32_t cl, uint32_t fl) {
  if (at == -1) {
    uint16_t * p0    = (uint16_t *)&m_prog[1];
    uint16_t * pc    = 0 + p0;
    at = *pc;
  }
  uint16_t * p0    = (uint16_t *)&m_prog[0];
  uint32_t * code  = (uint32_t *)&m_prog[p0[7]];
  uint32_t pv = code[at];
  uint32_t oper = pv >> 24;
  uint32_t op1 = (pv & 0x00fff000) >> 12;
  uint32_t op2 = (pv & 0x00000fff);
  char *opname = opnames[oper];
  char opbuf[20];
  if (strlen(opname) == 0) {
    sprintf(opname, "[0x%02x]", oper);
  }
  if (fl & FLAGSUPER)
      printf("                          ");

  printf("0x%03x %6s 0x%03x 0x%03x (clk: 0x%x) (F:%03x)\n",
    at, opname, op1, op2, cl, fl);
}

void Exec::list() {
  show(0, 0, 0);
  for (uint32_t i = 0; i < 14; i+=2) {
    uint32_t w = m_prog[1 + i/2];
    uint32_t left  = (w & 0x00fff000) >> 12;
    uint32_t right = (w & 0x00000fff);
    char *left_name = regs[i];
    char *right_name = regs[i+1];
    printf("0x%04x: %6s 0x%03x 0x%03x  %-10s %-10s\n",
      1 + i/2, "CPU", left, right, right_name, left_name);
  }
  for (uint32_t i = 8; i < m_sz; i++) {
    show(i, 0, 0);
  }
}

void Exec::load() {
  //
  // get references to CPU addresses
  uint16_t * p0    = (uint16_t *)&m_prog[0];
  // FIXME: bounds check these offsets
  uint16_t * exec  = (uint16_t *)&m_prog[p0[1]];
  uint16_t * data  = (uint16_t *)&m_prog[p0[3]];
  uint16_t * heap  = (uint16_t *)&m_prog[p0[5]];
  uint16_t * code  = (uint16_t *)&m_prog[p0[7]];
  //printf("%d %d %d %d\n", *exec, *data, *heap, *code);
  //exit(3);
  uint16_t * pc    = 0 + exec;
  *pc = 0;
}

uint16_t Exec::clock() {
  uint16_t * p0    = (uint16_t *)&m_prog[0];
  uint16_t * exec  = (uint16_t *)&m_prog[p0[1]];
  uint16_t * clock = 3 + exec;
  return *clock;
}

void Exec::execute(uint32_t ticks, Yield& y) {
  //
  // get references to CPU addresses
  // FIXME: unneccesary and slow.  make macros, and do these lazily
  uint16_t * p0    = (uint16_t *)&m_prog[0];
  // FIXME: bounds check these offsets
  uint16_t * exec  = (uint16_t *)&m_prog[p0[1]];
  uint32_t * data  = (uint32_t *)&m_prog[p0[3]];
  uint8_t  * heap  = (uint8_t  *)&m_prog[p0[5]];
  uint32_t * code  = (uint32_t *)&m_prog[p0[7]];
  //printf("%d %d %d %d\n", *exec, *data, *heap, *code);
  //exit(3);
  uint16_t * pc    = 0 + exec;
  uint16_t * flags = 1 + exec;
  uint16_t * user  = 2 + exec;
  uint16_t * clock = 3 + exec;
  uint16_t * frame = 4 + exec;
  uint16_t * color = 5 + exec;
  uint16_t * status = 6 + exec;
  uint16_t * action = 7 + exec;
  uint16_t * sense_food  = 8 + exec;
  uint16_t * sense_enemy = 9 + exec;
  uint16_t * health = 10 + exec;
  uint16_t * energy = 11 + exec;
  uint16_t * pc2    = 12 + exec;
  uint16_t * flags2 = 13 + exec;

  uint16_t last_clock = *clock + ticks;
  uint16_t clock0 = *clock;

  y.reason(0);
  while (!(*flags & FLAGHALT) && !y.reason()) {
    uint32_t op = code[*pc];
    uint8_t opcode = (op & 0xff000000) >> 24;
    uint16_t op1   = (op & 0x00fff000) >> 12;
    uint16_t op2   = (op & 0x00000fff);
    if (m_verbose) {
#if 0
      //if (*flags & FLAGSUPER)
      //  printf("                          ");
      //printf("pc   : 0x%03x %6s 0x%03x 0x%03x (clk: 0x%x) (F:%03x)\n",
      //      *pc, opnames[opcode], op1, op2, *clock, *flags);
      if (*flags & FLAGSUPER)
        printf("                          ");
      show(*pc, *clock, *flags);
#endif
    }
    switch(opcode) {
      case ZERO:
        data[op2] = 0;
        (*clock)++;
        break;
      case INCR:
        data[op2]++;
        (*clock)++;
        break;
      case TESTMC:
        *flags = 0;
        if (data[op1] == op2) (*flags) |= FLAGEQ;
        if (data[op1] >  op2) (*flags) |= FLAGGT;
        if (data[op1] <  op2) (*flags) |= FLAGLT;
        *clock += 3;
        break;
      case JUMP:
        (*pc) = op2 - 1;
        (*clock)++;
        break;
      case JUMPEQ:
        if ((*flags) & FLAGEQ)
          (*pc) = op2 - 1;
        (*clock)++;
        break;
      case JUMPNE:
        if (!((*flags) & FLAGEQ))
          (*pc) = op2 - 1;
        (*clock)++;
        break;
      case USEFRAME:
        if (op1 == 0xfff) {
          (*frame) = 0xfff;
          (*flags) &= ~FLAGFRAME;
        } else {
          (*frame) = op2;
          (*flags) |= FLAGFRAME;
        }
        (*clock)++;
        break;
      case BREAK:
        (*flags) = 0;
        (*user)  = 2;
        (*pc) = op2 - 1;
        *clock += 8; //??
        break;
      case DO:
        //printf("DO\n");
        //*flags |= FLAGSUPER;
        if (((*flags) & FLAGSUPER) && op1 == EAT) {
          if (drand48() > 0.8) {
            *pc = *pc2;
            *flags = *flags2;
            *flags &= (*flags) & ~FLAGSUPER;
            *user = 1;
          }
        }
        switch(op1) {
          case MOVE:
            y.reason(MOVE);
            break;
          default:
            break;
        }
        *clock += 50;
        break;
      case DOMOD:
        //printf("DOMOD %d %d\n", op1, op2);
        switch(op1) {
          case SAY:
            if (true) {
              char s[80];
              uint8_t *p = &heap[op2];
              for (uint32_t ix = 0; *p != '\t' && ix < sizeof(s) - 1; ix++, p++) {
                //printf("%c", *p);
                s[ix] = *p;
                s[ix + 1] = '\0';
              }
              //printf("\n");
              y.reason(SAY);
              y.text(s);
            }
            break;
          case TURN:
            y.reason(TURN);
            y.reason(TURN | op2 << 8);
            break;
        }
        *clock += 50;
        break;
      case SENSE:
        if (1) {
          uint16_t amount = (op1 & 0xf00) >> 8;
          uint16_t sense  = (op1 & 0x0f0) >> 4;
          uint16_t dir    = op2;
          uint16_t dist   = (op1 & 0x00f);
          if (drand48() > 0.5)
            *flags ^= FLAGEQ;
          amount + sense + dir + dist;
          *clock += 10;
        }
        break;
      case RETURN:
        if (*flags & FLAGSUPER) {
          *pc = *pc2;
          *flags = *flags2 & ~FLAGSUPER;
          *user = 1;
          *clock += 3;
        } else {
          printf("ERR.1 (done)\n");
          *flags |= FLAGHALT;
          continue;
        }
        break;
      default:
        printf("ERR.2 [%d]\n", opcode);
        *flags |= FLAGHALT;
        continue;
        break;
    }
    (*pc)++;
    //uint16_t diff = *clock - last_clock;
    //usleep(10000 * diff);
    if (*clock > last_clock)
      break;
    if (!(*flags & FLAGSUPER) && (*flags & FLAGFRAME)) {
      if (*user == 0) {
        *pc2 = *pc - 1;
        *flags2 = *flags;
        *flags |= FLAGSUPER;
        *pc = *frame;
      } else {
        (*user)--;
      }
    }
  }
  return;
}



int main(int argc, char** argv) {
  int prog[10240];
  uint32_t i = 0;
  if (argc == 1) {
    // no params, read from stdin
    std::string line;
    while (getline(std::cin, line)) {
      //printf("[%d]\n", atoi(line.c_str()));
      prog[i++] = atoi(line.c_str());
    }
  }
  if (argc == 2) {
    std::ifstream inf(argv[1], std::ios::in);
    std::string line;
    while (getline(inf, line)) {
      //printf("[%d]\n", atoi(line.c_str()));
      prog[i++] = atoi(line.c_str());
    }
  }
#if 0
  printf("the list:\n");
  for (uint32_t i=0; i<sizeof(prog)/sizeof(int); i++) {
    printf("%d %d\n", i, prog[i]);
    //if (i>10) break;
  }
#endif

#if 1
  Exec * e = new Exec(prog, i);
  e->verbose(true);
  Arena * a = new Arena();
  a->add(e);
  uint32_t r = 1;
  for (uint32_t i = 0; r && i < 195; i++) {
    r = a->step();
  }
  exit(0);
#else

  /*
   * tm 5187.820 ms  aggregate 1215752192 pCyc  4.687 M 'moves' per second 
   *
   * 1000 ticks = 20 moves - in 5 seconds, this is real-time
   * we do this 1000 times more often with 100,000 agents
   *
   */
  std::vector<Exec *> elist;
  uint64_t vmcount = 1000000; // 1000000;
  uint64_t loops = 30;
  uint64_t ticks = 100;
  printf("allocating at least: %3.1f MB\n", double(sizeof(prog2) * vmcount)/1024/1024);
  for (uint32_t i = 0; i < vmcount; i++) {
    Exec * e = new Exec(prog2, sizeof(prog2)/sizeof(int));
    elist.push_back(e);
  }
  //sleep(30);
#if 0
  elist[0]->list();
  printf("\n");
#endif
  double a = getTime();
  uint64_t tk =0;
  for (uint32_t j = 0; j < loops; j++) {
    for (uint32_t k = 0; k < vmcount; k++) {
      uint32_t m = lrand48() % ticks;
      //if (m & 1) continue;
      //m = ticks;
      tk += m + elist[k]->execute(m);
    }
  }
  double b = getTime();
  std::vector<Exec *>::iterator it = elist.begin();
  for (uint32_t i = 0 ; it != elist.end(); ++it, ++i) {
    printf("> %p\t", (*it)); 
    (*it)->show(-1);
    if (i > 20) break;
  }

  //elist[0]->list();
  printf("tm %3.3f ms  aggregate %lld pCyc  %3.3f M 'moves' per second \n",
    (b - a) * 1000, tk,
    double(tk) / (b - a) / 1000 / 1000 / 50);
#endif
}
