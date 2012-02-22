#ifndef __program_h__
#define __program_h__

#include <sstream>
#include <string.h>
#include "log.h"

namespace dj {

/*
 * we have a text version, and a tokenized version
 * when we 'run'
 */

  class Program {
   public:
    Program() {}
    ~Program() {}

    uint32_t editKey(int key, uint8_t sca, const char *dc, const char *cc, const char* co) 
    {
      return m_editor.editKey(key, sca, dc, cc, co);
    }
    const char *display_text() { 
      return m_editor.text(); 
    }
    void click(uint32_t row, uint32_t col) {
      m_editor.click(row, col);
    }
    void interpret() {
      m_interpreter.validate(m_editor.rawtext());
    }


   /**** editor ****/
    class Editor {
     public:
      Editor() : m_c_row(0), m_c_col(0), m_insertmode(true), m_s_row(-1) {
        memset(buf, ' ', sizeof(buf));
        memset(m_line_paste, '\0', sizeof(m_line_paste));
      }
     public:
      // apply user input to our "file"
      uint32_t editKey(int key, uint8_t sca, const char *dc, const char *cc, const char* co);

      // cursor by mouse click
      void click(uint32_t row, uint32_t col);

      // generate display-friendly (marked up) version of output
      const char *text();

      // generate verbatim version of output
      const char *rawtext();

      // save
      // load

     private:
      enum { BS = 8, TAB = 9, RETURN = 13, ESC = 27, PGUP = 33, PGDOWN = 34,
        END = 35, HOME = 36, LEFT = 37, UP = 38, RIGHT = 39, DOWN = 40,
        INSERT = 45, DELETE = 46, F1 = 112, F2 = 113 };

     private:
      char * at(uint32_t row, uint32_t col);
      uint32_t last_col(uint32_t row);
      void squeeze(uint32_t row, uint32_t col, uint32_t count);
      int expand(uint32_t row, uint32_t col, uint32_t count);
      int openline(uint32_t row);
      int squeezeline(uint32_t row);

     private:
      static const uint32_t cols = 12;
      static const uint32_t rows = 30;
      char buf[cols * rows];
      char m_line_paste[cols + 1]; // TODO: sizeme
      int32_t m_c_row;
      int32_t m_c_col;
      int32_t m_s_row; // -1 for no selection
      int32_t m_s_col;
      bool m_insertmode;
    };


   /**** interpreter ****/
   public:
    class Interpreter {
     public:
      Interpreter() {}
      ~Interpreter() {}

     private:
      enum { MOVE = 1, STOP, SPAWN, DIE, LEFT, RIGHT };

     public:
      // test and build tokenized tree
      uint32_t validate(const char *s);

      // take a step
      // require sensory callbacks?
      uint32_t step(uint32_t n);


     private:

    };
    

   private:
    Program::Editor m_editor;
    Program::Interpreter m_interpreter;
  };

} // namespace
#endif

