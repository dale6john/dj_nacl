#ifndef __bitmap_h__
#define __bitmap_h__

namespace dj {

class Bitmap {
 public:
  Bitmap() : m_v(NULL), m_steps(0) {}
  Bitmap(const uint32_t* v) { init(v); }
  Bitmap(uint32_t* v, uint32_t steps, uint32_t cols, uint32_t rows) 
      : m_v(NULL), m_base(v),
        m_steps(steps), m_elements(rows*cols), 
        m_columns(cols), m_rows(rows)
  {
  }
  ~Bitmap() {}

  void init(const uint32_t* v) { 
    m_v = (uint32_t *)v;
    m_steps = *v++;
    m_elements = *v++ + 2;
    m_columns = *v++;
    m_base = (uint32_t *)v;
    m_rows = m_elements / m_columns;
  }
  uint32_t rows() const { return m_rows; }
  uint32_t columns() const { return m_columns; }
  uint32_t steps() const { return m_steps; }
  inline const uint32_t at(uint32_t x, uint32_t y, uint32_t step) const {
    //if (step > 2) step = 0;
    return m_base[m_elements * step + x + y * m_columns];
  }
  inline void set(uint32_t x, uint32_t y, uint32_t step, uint32_t v) {
    m_base[m_elements * step + x + y * m_columns] = v;
  }

 private:
  uint32_t * m_v;
  uint32_t * m_base;
  uint32_t m_steps;
  uint32_t m_elements;
  uint32_t m_columns;
  uint32_t m_rows;
};

#if 0
  class Numbers : public Bitmap {
   public:
    Numbers() : m_width(0), m_height(0) {}
    Numbers(const uint32_t* v, uint32_t width, uint32_t height)
        : Bitmap(v), m_width(width), m_height(height) {}
    ~Numbers() {}

   public:
    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    inline uint32_t at(uint32_t x, uint32_t y, uint32_t v) const {
      // assert m_width
      return Bitmap::at(v * m_width + x, y, 0);
    }
    // bitmap constructor
    Bitmap &digit(uint8_t digit) {
      // when new digit
      if (m_digits[digit].steps() == 0) {
        printf("recalc\n");
        uint32_t * buf = new uint32_t[ m_width * m_height ];
        memset(buf, '\0', sizeof(buf));
        m_digits[digit] = Bitmap(buf, 1, m_width, m_height);
        for (uint32_t x = 0; x < m_width; x++) {
          for (uint32_t y = 0; y < m_height; y++) {
            m_digits[digit].set(x, y, 0, Bitmap::at(digit * m_width + x, y, 0));
          }
        }
      }
      return m_digits[digit];
    }

   private:
    uint32_t m_width;
    uint32_t m_height;
    Bitmap m_digits[10];
  };
#endif

}

#endif
