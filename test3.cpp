#include "dj_two.h"
#include <sys/time.h>

using namespace dj;

#define OLD 0

void render_bitmap(Bitmap & map, uint32_t step) {
  uint32_t max_x = map.columns();
  uint32_t max_y = map.rows();
  for (uint32_t x = 0; x < max_x; x++) {
    printf("%d ", x / 10);
  }
  printf("\n");
  for (uint32_t x = 0; x < max_x; x++) {
    printf("%d ", x % 10);
  }
  printf("\n");
  for (int32_t y = max_y - 1; y >= 0; y--) {
    for (int32_t x = 0; x < int32_t(max_x); x++) {
      uint32_t color = map.at(x, y, step);
      //uint32_t alpha = (color & 0xff000000) >> 24;
      uint32_t red   = (color & 0x00ff0000) >> 16;
      uint32_t green = (color & 0x0000ff00) >> 8;
      uint32_t blue  = (color & 0x000000ff);
      if (red + green + blue < 30)
        printf("XX");
      else if (red + green + blue < 260)
        printf("**");
      else if (red + green + blue < 620)
        printf("++");
      else if (red + green + blue < 750)
        printf("..");
      else
        printf("  ");
    }
    printf(" %02d\n", y);
  }
}

void render_ascii_dw(Canvas & canvas, uint32_t max_x, uint32_t max_y) {
  for (uint32_t x = 0; x < max_x; x++) {
    printf("%d ", x / 10);
  }
  printf("\n");
  for (uint32_t x = 0; x < max_x; x++) {
    printf("%d ", x % 10);
  }
  printf("\n");
  for (int32_t y = max_y - 1; y >= 0; y--) {
    for (int32_t x = 0; x < int32_t(max_x); x++) {
      uint32_t color = canvas.get(x, y);
      //uint32_t alpha = (color & 0xff000000) >> 24;
      uint32_t red   = (color & 0x00ff0000) >> 16;
      uint32_t green = (color & 0x0000ff00) >> 8;
      uint32_t blue  = (color & 0x000000ff);
      if (red + green + blue < 30)
        printf("XX");
      else if (red + green + blue < 260)
        printf("**");
      else if (red + green + blue < 620)
        printf("++");
      else if (red + green + blue < 750)
        printf("..");
      else
        printf("  ");
    }
    printf(" %02d\n", y);
  }
}

void render_ascii(Canvas & canvas, uint32_t max_x, uint32_t max_y) {
  for (uint32_t x = 0; x < max_x; x++) {
    printf("%d", x / 10);
  }
  printf("\n");
  for (uint32_t x = 0; x < max_x; x++) {
    printf("%d", x % 10);
  }
  printf("\n");
  for (int32_t y = max_y - 1; y >= 0; y--) {
    for (int32_t x = 0; x < int32_t(max_x); x++) {
      uint32_t color = canvas.get(x, y);
      //uint32_t alpha = (color & 0xff000000) >> 24;
      uint32_t red   = (color & 0x00ff0000) >> 16;
      uint32_t green = (color & 0x0000ff00) >> 8;
      uint32_t blue  = (color & 0x000000ff);
      if (red + green + blue < 30)
        printf("X");
      else if (red + green + blue < 260)
        printf("*");
      else if (red + green + blue < 620)
        printf("+");
      else if (red + green + blue < 750)
        printf(".");
      else
        printf(" ");
    }
    printf(" %02d\n", y);
  }
}

inline double getTime() {
  timeval tm;
  gettimeofday( &tm, NULL );
  return (double) tm.tv_sec + ( (double) tm.tv_usec ) / 1000000.0;
}

int main(int argc, char **argv) {
  /* skipping GameState completely for now */
  uint32_t canvas_x = 800;
  uint32_t canvas_y = 600;

  double view_scale = 1.0;
  //double view_scale = 0.75; // zoom out a bit
  //double view_scale = 1.67; // zoom in a bit
  dj::View view(canvas_x, canvas_y, view_scale);
  //view.center(60, 90);
  view.center(0, 0);
  //Numbers num(numbers, 12, 16);
  //DisplayContext dc(&view, &num);

  uint32_t pixel_bits[canvas_x * canvas_y];
  Canvas canvas(pixel_bits, canvas_x, canvas_y);
  canvas.clear();

  uint32_t px[800 * 600 + 100];
  GameState game(800, 600, 1.0);
  for (uint32_t i = 0; i < 2000; i++) {
    if (i % 100 == 0)
      printf("Step %d\n", i);
    game.step();
    //game.getTurn();
    game.redraw(px);
  }

  exit(0);
}
