#pragma once

#include <array>

// pieces and rotations come from:
// they are 4x4 clockwise rotating
// https://tetris.wiki/File:SRS-pieces.png
const std::array<int, 4> tower = {{
    0b1111000000000000,
    0b0010001000100010,
    0b0000000011110000,
    0b0100010001000100,
}};

const std::array<int, 4> box = {{
    0b0110011000000000,
    0b0110011000000000,
    0b0110011000000000,
    0b0110011000000000,
}};

const std::array<int, 4> pyramid = {{
    0b0100111000000000,
    0b0100011001000000,
    0b0000111001000000,
    0b0100110001000000,
}};

const std::array<int, 4> leftlean = {{
    0b1100011000000000,
    0b0010011001000000,
    0b0000110001100000,
    0b0100110010000000,
}};

const std::array<int, 4> rightlean = {{
    0b0110110000000000,
    0b0100011000100000,
    0b0000011011000000,
    0b1000110001000000,
}};

const std::array<int, 4> leftknight = {{
    0b1000111000000000,
    0b0110010001000000,
    0b0000111000100000,
    0b0100010011000000,
}};

const std::array<int, 4> rightknight = {{
    0b0010111000000000,
    0b0110010001000000,
    0b0000111000100000,
    0b0100010011000000,
}};

inline std::array<int, 16> bit_to_array(int b) {
  std::array<int, 16> tmp;
  int j = 0;
  for (int i = 15; i >= 0; i--) {
    tmp[j++] = (b >> i) & 1;
  }
  return tmp;
}

inline std::array<int, 16> type_to_rotated_array(int t, int a) {
  if (t == 0)
    return bit_to_array(tower[a]);
  if (t == 1)
    return bit_to_array(box[a]);
  if (t == 2)
    return bit_to_array(pyramid[a]);
  if (t == 3)
    return bit_to_array(leftlean[a]);
  if (t == 4)
    return bit_to_array(rightlean[a]);
  if (t == 5)
    return bit_to_array(leftknight[a]);
  if (t == 6)
    return bit_to_array(rightknight[a]);
  return bit_to_array(0);
}

const std::array<std::array<std::pair<int, int>, 4>, 4> long_boi_tests = {{
    {{
        {-2, +0},
        {+1, +0},
        {-2, -1},
        {+1, +2},
    }},
    {{
        {-1, +0},
        {+2, +0},
        {-1, +2},
        {+2, -1},
    }},
    {{
        {+2, +0},
        {-1, +0},
        {+2, +1},
        {-1, -2},
    }},
    {{
        {+1, +0},
        {-2, +0},
        {+1, -2},
        {-2, +1},
    }},
}};

std::array<std::array<std::pair<int, int>, 4>, 4> wall_kick_tests = {{
    {{
        {-1, +0},
        {-1, +1},
        {-0, -2},
        {-1, -2},
    }},
    {{
        {+1, +0},
        {+1, -1},
        {+0, +2},
        {+1, +2},
    }},
    {{
        {+1, +0},
        {+1, +1},
        {+0, -2},
        {+1, -2},
    }},
    {{
        {-1, +0},
        {-1, -1},
        {+0, +2},
        {-1, +2},
    }},
}};
