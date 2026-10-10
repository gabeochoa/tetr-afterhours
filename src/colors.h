
#pragma once

#include "rl.h"

namespace color {
using raylib::Color;

const Color BLACK_ = Color{51, 51, 51, 255};
const Color BACKGROUND = Color{22, 22, 30, 255};
const Color PANEL = Color{38, 38, 50, 255};
const Color EMPTY_CELL = Color{52, 52, 68, 255};
const Color LABEL = Color{140, 140, 165, 255};
const Color GRAY_ = Color{128, 128, 128, 255};
const Color WHITE_ = Color{255, 255, 255, 255};
const Color CYAN_ = Color{80, 220, 230, 255};
const Color YELLOW_ = Color{245, 210, 70, 255};
const Color PURPLE_ = Color{170, 90, 220, 255};
const Color GREEN_ = Color{110, 210, 90, 255};
const Color RED_ = Color{235, 85, 85, 255};
const Color BLUE_ = Color{80, 120, 235, 255};
const Color ORANGE_ = Color{245, 150, 60, 255};

inline Color piece_color(int type) {
  if (type == 0)
    return CYAN_;
  if (type == 1)
    return YELLOW_;
  if (type == 2)
    return PURPLE_;
  if (type == 3)
    return GREEN_;
  if (type == 4)
    return RED_;
  if (type == 5)
    return BLUE_;
  if (type == 6)
    return ORANGE_;
  if (type == -1)
    return GRAY_;

  return WHITE_;
}

} // namespace color
