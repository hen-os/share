#include "figfont.h"

int main() {
  FigFont *font = ReadFigFont("slant.flf");
  printFigFont(font, "text");
  return 0;
}
