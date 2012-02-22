#include "program.h"

namespace dj {

  uint32_t Program::Editor::editKey(int key, uint8_t sca, const char *dc, const char *cc, const char* co) {
    theLog.info("%d/ sca:%d dc/cc/co: %s/%s/%s",
        key, sca, dc, cc, co);
    if (dc && dc[0] && !(sca & 0x2)) {
      if (m_c_col < cols) {
        // is displayable
        uint32_t use_key = (sca & 0x1) ? dc[1] : dc[0];
        if (m_insertmode) {
          if (expand(m_c_row, m_c_col, 1))
            return 7;
        }
        *at(m_c_row, m_c_col) = use_key;
        m_c_col++;
      } else {
        return 1; // bell??
      }
    } else if (sca & 0x2) {
      switch(key) {
        case 'C':
          if (1) {
            uint32_t sel0 = m_s_col < m_c_col ? m_s_col : m_c_col - 1;
            uint32_t sel1 = m_s_col > m_c_col ? m_s_col : m_c_col - 1;
            strncpy(m_line_paste, at(m_s_row, sel0), sel1 - sel0 + 1);
            m_s_row = -1;
            m_c_col = sel0;
            theLog.info("copy: '%s'", m_line_paste);
          }
          break;
        case 'V':
          theLog.info("paste: '%s' insertmode: %d", m_line_paste, m_insertmode);
          if (1) {
            uint32_t chars = strlen(m_line_paste);
            if (m_insertmode) {
              if (strlen(m_line_paste) + last_col(m_c_row) <= cols) {
                expand(m_c_row, m_c_col, chars);
                memcpy(at(m_c_row, m_c_col), m_line_paste, chars);
                m_c_col += chars;
              } else {
                return 10;
              }
            } else {
              memcpy(at(m_c_row, m_c_col), m_line_paste, chars);
              m_c_col += chars;
            }
          }
          break;
        case 'X':
          if (1) {
            uint32_t sel0 = m_s_col < m_c_col ? m_s_col : m_c_col - 1;
            uint32_t sel1 = m_s_col > m_c_col ? m_s_col : m_c_col - 1;
            strncpy(m_line_paste, at(m_s_row, sel0), sel1 - sel0 + 1);
            squeeze(m_s_row, sel0, sel1 - sel0 + 1);
            m_s_row = -1;
            m_c_col = sel0;
            theLog.info("cut: '%s' (%d/%d)", m_line_paste, sel0, sel1);
          }
          break;
        case 'S':
          // save is a "great big deal"
          theLog.info("save is a 'big deal'!!");
          break;
        case 'Z':
          // undo is a "great big deal"
          theLog.info("undo is a 'big deal'!!");
          break;
        case RETURN:
          if (sca & 0x2 || m_insertmode) {
            // FIXME: factor
            // shift down one row
            uint32_t last = last_col(m_c_row);
            if (openline(m_c_row))
              return 8;
            char *p = at(m_c_row, m_c_col);
            char *q = at(m_c_row, cols);
            uint32_t j = 0;
            while (p < q) {
              *at(m_c_row + 1, j++) = *p;
              *p++ = ' ';
            }
            m_c_col = 0;
            m_c_row++;
          } else {
            m_c_col = 0;
            m_c_row++;
          }
          break;
        default:
          theLog.info("NOT IMPL 2: %d/ sca:%d dc/cc/co: %s/%s/%s",
              key, sca, dc, cc, co);
          break;
      }
    } else {
      switch(key) {
        case BS: 
          if (m_c_col > 0) {
            m_c_col--;
            squeeze(m_c_row, m_c_col, 1);
          } else {
            // concat the two lines
            if (m_c_row > 0) {
              uint32_t last0 = last_col(m_c_row - 1);
              uint32_t last1 = last_col(m_c_row);
              if (last0 + last1 <= cols) {
                m_c_row--;
                char *p = at(m_c_row, last0);
                memcpy(p, at(m_c_row + 1, 0), cols);
                squeezeline(m_c_row + 1);
                m_c_col = last0;
              } else {
                return 2; // bell??
              }
            } else {
              return 2; // bell??
            }
          }
          break;
        case PGUP:
          m_c_col = 0;
          m_c_row = 0;
          break;
        case PGDOWN:
          m_c_col = 0;
          m_c_row = rows - 1;
          for (uint32_t i = 0; i < rows; i++) {
            if (m_c_row == 0 || last_col(m_c_row) > 0)
              break;
            m_c_row--;
          }
          break;
        case HOME:
          m_c_col = 0;
          break;
        case END:
          m_c_col = last_col(m_c_row);
          break;
        case DELETE: 
          squeeze(m_c_row, m_c_col, 1);
          break;
        case INSERT: 
          m_insertmode ^= true;
          //theLog.info("(r:%d c:%d) last col:%d", m_c_row, m_c_col, last_col(m_c_row));
          break;
        case RETURN:
          if (sca & 0x2 || m_insertmode) {
            // shift down one row
            uint32_t last = last_col(m_c_row);
            if (openline(m_c_row))
              return 8;
            char *p = at(m_c_row, m_c_col);
            char *q = at(m_c_row, cols);
            uint32_t j = 0;
            while (p < q) {
              *at(m_c_row + 1, j++) = *p;
              *p++ = ' ';
            }
            m_c_col = 0;
            m_c_row++;
          } else {
            m_c_col = 0;
            m_c_row++;
          }
          break;
        case UP:
          if (m_c_row > 0) {
            m_c_row--;
          } else {
            return 5;
          }
          break;
        case DOWN:
          if (m_c_row < rows - 1) {
            m_c_row++;
          } else {
            return 6;
          }
          break;
        case LEFT:
          if (sca & 0x1 && m_s_row == -1) {
            m_s_row = m_c_row;
            m_s_col = m_c_col - 1;
          } else if (!(sca & 0x1)) {
            m_s_row = -1;
          }
          if (m_c_col > 0) {
            m_c_col--;
          } else {
            m_c_row--;
            m_c_col = last_col(m_c_row);
          }
          break;
        case RIGHT:
          if (sca & 0x1 && m_s_row == -1) {
            m_s_row = m_c_row;
            m_s_col = m_c_col;
          } else if (!(sca & 0x1)) {
            m_s_row = -1;
          }
          if (m_c_col < cols) {
            m_c_col++;
          } else {
            if (m_c_row < rows - 1) {
              m_c_row++;
              m_c_col = 0;
            } else {
              return 4;
            }
          }
          break;
        default:
          //*at(m_c_col, m_c_row) = '@';
          theLog.info("NOT IMPL 1: %d/ sca:%d dc/cc/co: %s/%s/%s",
              key, sca, dc, cc, co);
          return 3;
          break;
      }
    }
    return 0;
  }

  void Program::Editor::click(uint32_t row, uint32_t col) {
    m_c_row = row;
    m_c_col = col;
    m_s_row = -1;
  }

  const char *Program::Editor::text() {
    std::stringstream oss;
    uint32_t sel0 = m_s_col < m_c_col ? m_s_col : m_c_col - 1;
    uint32_t sel1 = m_s_col > m_c_col ? m_s_col : m_c_col - 1;
    for (uint32_t i = 0; i < rows; i++) {
      uint32_t last = last_col(i);
      if (i == m_c_row && last <= m_c_col)
        last = m_c_col + 1;
      for (uint32_t j = 0; j < last; j++) {
        if (i == m_c_row && j == m_c_col) {
          if (m_insertmode)
            oss << "*+";
          else
            oss << "*-";
        }
        if (i == m_s_row && j == sel0)
          oss << "*(";
        if (j >= cols)
          oss << " ";
        else
          oss << *at(i, j);
        if (i == m_s_row && j == sel1)
          oss << "*)";
      }
      oss << "\r";
    }
#if 0
    const char *p = oss.str().c_str();
    char buf[10240];
    char *q = &buf[0];
    uint32_t sp = 0;
    while (*p) {
      if (*p != ' ' && *p != 13) { *q++ = *p; sp = 0; }
      if ((*p == ' ' || *p == 13 ) && sp == 0) { *q++ = ' '; sp++; }
      p++;
    }
    *q = '\0';
    theLog.info("[%s]", buf);
#endif
    return oss.str().c_str();
  }

  const char *Program::Editor::rawtext() {
    std::stringstream oss;
    for (uint32_t i = 0; i < rows; i++) {
      uint32_t last = last_col(i);
      for (uint32_t j = 0; j < last; j++) {
        oss << *at(i, j);
      }
      oss << ";";
    }
    return oss.str().c_str();
  }

  char * Program::Editor::at(uint32_t row, uint32_t col) {
    return &buf[row * cols + col];
  }
  uint32_t Program::Editor::last_col(uint32_t row) {
    char *p = &buf[row * cols];
    char *q = &buf[row * cols + cols - 1];
    while (*q == ' ' && p != q) q--;
    if (p == q && *p == ' ') 
      return 0;
    return 1 + q - p;
  }
  void Program::Editor::squeeze(uint32_t row, uint32_t col, uint32_t count) {
    char *p = &buf[row * cols + col];
    char *q = &buf[row * cols + cols - count];
    char *e = &buf[row * cols + cols - 1];
    while (p < q) {
      *p = *(p + count);
      p++;
    }
    while (q <= e) {
      *q++ = ' ';
    }
  }
  int Program::Editor::expand(uint32_t row, uint32_t col, uint32_t count) {
    uint32_t last = last_col(row);
    if (cols - last < count)
      return 1;
    char *s = &buf[row * cols + col];
    char *p = &buf[row * cols + col + count - 1];
    char *q = &buf[row * cols + cols - 1];
    while (p < q) {
      *q = *(q - count);
      q--;
    }
    while (p >= s) {
      *p-- = ' ';
    }
    return 0;
  }
  int Program::Editor::openline(uint32_t row) {
    if (last_col(rows - 1) > 0)
      return 1;
    for (uint32_t i = rows - 2; i > row; i--)
      memcpy(at(i + 1, 0), at(i, 0), cols);
    return 0;
  }
  int Program::Editor::squeezeline(uint32_t row) {
    for (uint32_t i = row; i < rows - 1; i++)
      memcpy(at(i, 0), at(i + 1, 0), cols);
    memset(at(rows - 1, 0), ' ', cols);
    return 0;
  }



  // actions m/ove s/top +/spawn -/die l/eft r/ight e/eat f/ight x/exit loop
  // 9m - move 9
  // @m - move forever
  // @{ml} - move, left forever
  // 5{ml} - five times move, left
  // L - look - f,F,e,E
  // E - event - e eat, attacked
  // f0 - food in front of me
  // f<1 - food slightly left
  // f<2 - food somewhat left
  // f<3 - food left peripheral
  // f>1 - food slightly right
  // f0?m - food somewhat far in front
  // F0?m - food just in front
  // F0?:2l - food not in front, rotate left 2
  //
  // @{mmL/F0|f0?:x/}  while food in front, move ahead
  // @{L/F0|f0?:{sx}/F>1?l/F<1?r/ mL}
  // @{L/F0|f0?:{sx}/F>1?l/F<1?r/F0?e E/e?+/a?5r/ mL}
  // @{ Look/
  //      FOOD 0 or food 0 ? NOOP :
  //      { stop; exit }
  //    / FOOD > 1 ?
  //        left
  //    / FOOD < 1 ?
  //        right
  //    / FOOD 0 ?
  //        eat
  //    Event/
  //      ate ?
  //        spawn
  //    / attacked ?
  //        5 times run
  //    /
  //    move
  //    look
  //  }
  //

  /*
   * movement/action
   *  move: m m< m> 
   *  run:  r r> r<
   *  poop: p
   *  turn: t< t>
   *  wait: w
   *  eat:  e e< e>
   *  fight: f f< f>
   *  spawn: s
   *  die:   d
   * sensory interrupts:
   *  food:   f F
   *  enemy:  e E
   *  attack: a A  (being attacked)
   *   (listen, feel, smell)
   * grammatical:
   *  if:     COND ? ACTION : ACTION
   *  block:  '{' ACTION* '}'
   *  repeat: NN ( ACTION | block )
   *  loop:   '@' ( ACTION | block )
   *  exit:   x
   * memory:
   *  set:    ($L=NN)
   *  incr:   ($L++) ($L--)
   *  test:   > < >= <= ==
   * relative location:
   *  heading:  ! < > << >>  (11 to 1), (9 to 11), (1 to 3), more
   *  distance: . _ - ^
   * appearance:
   *  state:  S/state description/
   *  color:  C/color/
   *
   * grammar:
   *  program: STATEMENTS(s)
   *  statement: COUNT
   *  listener: '(' SENSORY SENSORY_MOD '?' STATEMENT(s) ':' STATEMENTS(s) ')'
   *  BLOCK: set of interrupts and an action list
   *
   *
   * 5f 6{2m>p} 
   * 12{
   *   (f!_?e)
   *   (f<!>-?3r s:3t> m)
   *   (e!_?f)
   *   (E!_?2t>3r)
   *   (E!-?$panic=1;x)
   *   6m
   * }
   * @{
   *   ($panic==0;?x)
   *   (E!?3t>3r:$panic=0;)
   *   m>
   * }
   *
   */

  uint32_t validate(const char* s) {
    theLog.info("[%s]", s);
    return 0;
  }
}
