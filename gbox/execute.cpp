#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <vector>

char opnames[][12] = {
"",
"SENSE",
"ZERO",
"INCR",
"BREAK",
"",
"DO",
"DOMOD",
"RETURN",
"",
"",
"",
"",
"",
"",
"USEFRA",
"TESTMM",
"TESTCM",
"TESTMC",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"JUMP",
"JUMPEQ",
"JUMPNE" };

char regs[][15] = {
"_pc",
"_flags",
"_user",
"_clock",
"_frame",
"_color",
"_status",
"_action",
"_sense_food",
"_sense_enemy",
"_health",
"_energy",
"_pc2",
"_flags2"
};

int prog2[] = {
          553644041,
          0,
          0,
          0,
          0,
          0,
          0,
          0,
          0,
          268435455,
          50327618,
          100667392,
          67104834,
          302260226,
          570425355,
          268435455,
          117465090,
          268435455,
          117465089,
          251658297,
          50327619,
          100671488,
          67104835,
          302264323,
          570425365,
          50327620,
          251658297,
          117444610,
          268435455,
          67104836,
          302268419,
          570425370,
          50327621,
          100671488,
          67104837,
          302272515,
          570425377,
          50327622,
          268435455,
          117444609,
          268435455,
          67104838,
          302276611,
          570425382,
          268435455,
          50327623,
          268435455,
          117444610,
          100671488,
          268435455,
          67104839,
          302280724,
          570425390,
          268435455,
          536870921,
          134217728,
          134217728,
          17895438,
          570425409,
          100671488,
          50327624,
          100679680,
          67104840,
          302284803,
          570425405,
          134217728,
          0,
          0,
          0,
          0,
          0,
          0,
          0
};

int prog[] = {
553644041,
0,
0,
0,
0,
0,
0,
0,
0,
268435455,
50327598,
100667392,
67104814,
302178306,
570425355,
268435455,
117465090,
268435455,
117465089,
251658275,
50327599,
100671488,
67104815,
302182403,
570425365,
251658279,
100679680,
536870938,
268435455,
117444609,
268435455,
117444610,
268435455,
134217728,
134217728,
17895438,
570425382,
100671488,
134217728,
16871455,
570425386,
117444610,
17895438,
570425389,
100671488,
134217728,
0,
};


double getTime() {
  timeval tm;
  gettimeofday( &tm, NULL );
  return (double) tm.tv_sec + ( (double) tm.tv_usec ) / 1000000.0;
}


class Exec {
 public:
  Exec(int *prog, int sz) {
    //m_prog = prog;
    m_prog = new int [sz + 1];
    memcpy((char *)m_prog, (char *)prog, sz * sizeof(int));
    m_sz = sz;
  }
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
static const uint16_t FLAGBREAK = 0x008;  // not used
static const uint16_t FLAGFRAME = 0x010;  // enable frame interrupts
static const uint16_t FLAGSUPER = 0x020;

static const uint16_t TURN = 1;
static const uint16_t MOVE = 2;
static const uint16_t FIGHT = 3;
static const uint16_t EAT = 4;
static const uint16_t EXIT = 5;
static const uint16_t COLOR = 6;

  void show(int at) {
    if (at == -1) {
      uint16_t * p0    = (uint16_t *)&m_prog[1];
      uint16_t * pc    = 0 + p0;
      at = *pc;
    }
    uint32_t pv = m_prog[at];
    uint32_t oper = pv >> 24;
    uint32_t op1 = (pv & 0x00fff000) >> 12;
    uint32_t op2 = (pv & 0x00000fff);
    printf("0x%04x: %6s 0x%03x 0x%03x\n",
      at, opnames[oper], op1, op2);
  }
  void list() {
    show(0);
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
      show(i);
    }
  }
  void execute(uint32_t ticks) {
    //
    // get references to CPU addresses
    uint16_t * p0    = (uint16_t *)&m_prog[1];
    uint16_t * pc    = 0 + p0;
    uint16_t * flags = 1 + p0;
    uint16_t * user  = 2 + p0;
    uint16_t * clock = 3 + p0;
    uint16_t * frame = 4 + p0;
    uint16_t * color = 5 + p0;
    uint16_t * status = 6 + p0;
    uint16_t * action = 7 + p0;
    uint16_t * sense_food  = 8 + p0;
    uint16_t * sense_enemy = 9 + p0;
    uint16_t * health = 10 + p0;
    uint16_t * energy = 11 + p0;
    uint16_t * pc2    = 12 + p0;
    uint16_t * flags2 = 13 + p0;

    uint16_t last_clock = *clock;

    while (true) {
#if 0
      //printf("pc   : 0x%03x %p\n", *pc, pc);
      //printf("flags: 0x%03x %p\n", *flags, flags);
      if (*flags & FLAGSUPER)
        printf("                          ");
      show(*pc);
#endif
      uint32_t op = m_prog[*pc];
      uint8_t opcode = (op & 0xff000000) >> 24;
      uint16_t op1   = (op & 0x00fff000) >> 12;
      uint16_t op2   = (op & 0x00000fff);
      switch(opcode) {
        case ZERO:
          m_prog[op2] = 0;
          (*clock)++;
          break;
        case INCR:
          m_prog[op2]++;
          (*clock)++;
          break;
        case TESTMC:
          if (m_prog[op1] == op2) (*flags) |= FLAGEQ;
          if (m_prog[op1] >  op2) (*flags) |= FLAGGT;
          if (m_prog[op1] <  op2) (*flags) |= FLAGLT;
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
          if (((*flags) & FLAGSUPER) && op1 == EAT) {
            if (drand48() > 0.8) {
              *pc = *pc2;
              *flags = *flags2;
              *flags &= (*flags) & ~FLAGSUPER;
              *user = 1;
            }
          }
          *clock += 50;
          break;
        case DOMOD:
          //printf("DOMOD\n");
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
            printf("ERR.1\n");
            exit(3);
          }
          break;
        default:
          printf("ERR.2 [%d]\n", opcode);
          exit(3);
          break;
      }
      (*pc)++;
      uint16_t diff = *clock - last_clock;
      //usleep(10000 * diff);
      last_clock = *clock;
      if (last_clock > ticks)
        return;
      if ((*flags & FLAGSUPER) && (*flags & FLAGFRAME)) {
        if (*user == 0) {
          *pc2 = *pc - 1;
          *flags2 = *flags;
          *flags = *flags | FLAGSUPER;
          *pc = *frame;
        } else {
          (*user)--;
        }
      }
    }
  }
 private:
  int *m_prog;
  uint32_t m_sz;
};

int main(int argc, char** argv) {
#if 0
  printf("the list:\n");
  for (uint32_t i=0; i<sizeof(prog)/sizeof(int); i++) {
    printf("%d %d\n", i, prog[i]);
    //if (i>10) break;
  }
#endif

  /*
   * tm 5187.820 ms  aggregate 1215752192 pCyc  4.687 M 'moves' per second 
   *
   * 1000 ticks = 20 moves - in 5 seconds, this is real-time
   * we do this 1000 times more often with 100,000 agents
   *
   */
  std::vector<Exec *> elist;
  uint64_t vmcount = 1000000; // 1000000;
  uint64_t loops = 1;
  uint64_t ticks = 3000;
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
      elist[k]->execute(m);
      tk += m;
    }
  }
  double b = getTime();
  std::vector<Exec *>::iterator it = elist.begin();
  for (uint32_t i = 0 ; it != elist.end(); ++it, ++i) {
    printf("%p\t", (*it)); 
    (*it)->show(-1);
    if (i > 20) break;
  }

  //elist[0]->list();
  printf("tm %3.3f ms  aggregate %lld pCyc  %3.3f M 'moves' per second \n",
    (b - a) * 1000, tk,
    (tk) / (b - a) / 1000 / 1000 / 50);
}
