#ifndef __program_h__
#define __program_h__

#include <sstream>

namespace dj {

  enum { BS = 8, TAB = 9, RETURN = 13, ESC = 27, PGUP = 33, PGDOWN = 34,
    END = 35, HOME = 36, LEFT = 37, UP = 38, RIGHT = 39, DOWN = 40,
    INSERT = 45, DELETE = 46, F1 = 112, F2 = 113 };

/*
 * we have a text version, and a tokenized version
 * when we 'run'
 */

  class Program {
   public:
    Program() : m_c_row(0), m_c_col(0), m_insertmode(false), m_s_row(-1) {
      memset(buf, ' ', sizeof(buf));
    }
    ~Program() {}

    char * at(uint32_t row, uint32_t col) {
      return &buf[row * cols + col];
    }
    uint32_t last_col(uint32_t row) {
      char *p = &buf[row * cols];
      char *q = &buf[row * cols + cols - 1];
      while (*q == ' ' && p != q) q--;
      if (p == q && *p == ' ') 
        return 0;
      return 1 + q - p;
    }
    void squeeze(uint32_t row, uint32_t col) {
      char *p = &buf[row * cols + col];
      char *q = &buf[row * cols + cols - 1];
      while (p != q) {
        *p = *(p + 1);
        p++;
      }
      *q = ' ';
    }
    int expand(uint32_t row, uint32_t col) {
      char *p = &buf[row * cols + col];
      char *q = &buf[row * cols + cols - 1];
      if (buf[row * cols + cols - 1] != ' ')
        return 1; // warn if data loss
      while (p != q) {
        *q = *(q - 1);
        q--;
      }
      *p = ' ';
      return 0;
    }
    int openline(uint32_t row) {
      if (last_col(rows - 1) > 0)
        return 1;
      for (uint32_t i = rows - 2; i > row; i--)
        memcpy(at(i + 1, 0), at(i, 0), cols);
      return 0;
    }
    int squeezeline(uint32_t row) {
      for (uint32_t i = row; i < rows - 1; i++)
        memcpy(at(i, 0), at(i + 1, 0), cols);
      memset(at(rows - 1, 0), ' ', cols);
      return 0;
    }

    uint32_t editKey(int key, uint8_t sca, const char *dc, const char *cc, const char* co) {
      if (dc && dc[0]) {
        if (m_c_col < 12) {
          // is displayable
          uint32_t use_key = (sca & 0x1) ? dc[1] : dc[0];
          if (m_insertmode) {
            if (expand(m_c_row, m_c_col))
              return 7;
          }
          *at(m_c_row, m_c_col) = use_key;
          m_c_col++;
        } else {
          return 1; // bell??
        }
      } else {
        switch(key) {
          case BS: 
            if (m_c_col > 0) {
              m_c_col--;
              squeeze(m_c_row, m_c_col);
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
            squeeze(m_c_row, m_c_col);
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
              m_s_col = m_c_col;
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
            *at(m_c_col, m_c_row) = '@';
            return 3;
            break;
        }
      }
      return 0;
    }
    const char *text() {
      std::stringstream oss;
      uint32_t sel0 = m_s_col < m_c_col ? m_s_col : m_c_col;
      uint32_t sel1 = m_s_col > m_c_col ? m_s_col : m_c_col;
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
    int32_t cursor() {
      return -1;
    }

   private:
    static const uint32_t cols = 12;
    static const uint32_t rows = 30;
    char buf[cols * rows];
    int32_t m_c_row;
    int32_t m_c_col;
    int32_t m_s_row; // -1 for no selection
    int32_t m_s_col;
    bool m_insertmode;
  };

} // namespace
#endif

