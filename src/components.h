
#pragma once
// this is a positioned file
// so the includes are above in main.cpp

struct Transform : public BaseComponent {
  Transform(vec2 pos) : position(pos) {}

  virtual ~Transform() {}

  [[nodiscard]] vec2 pos() const { return position; }
  void update(vec2 v) { position = v; }

private:
  vec2 position;
};

struct HasCollision : public BaseComponent {};
struct IsGround : public BaseComponent {};
struct IsFalling : public BaseComponent {};

struct PieceType : public BaseComponent {
  PieceType(int t) : type(t), angle(0) {
    shape = type_to_rotated_array(type, angle);
  }
  int type;
  std::array<int, 16> shape;
  int angle;
};

struct NextPieceHolder : public BaseComponent {
  int next_type;
  NextPieceHolder() : next_type(rand() % 6) {}
};

// Where a falling piece is drawn: chases Transform::pos() on a spring so
// moves and drops slide instead of snapping.
struct SpringPos : public BaseComponent {
  struct Axis {
    motion::SpringState st;
    float t = 0;
  };
  Axis x, y;
  vec2 shown;
  SpringPos(vec2 pos) : x{{pos.x, 0, pos.x}}, y{{pos.y, 0, pos.y}}, shown(pos) {}
};

struct LineBurst : public BaseComponent {
  particles::Emitter<512> emitter;
  LineBurst() {
    emitter.gravity = {0, 900};
    emitter.floor_y = map_h * sz;
  }
};

struct Grid : public BaseComponent {
  int totalCleared = 0;
  std::array<std::array<int, map_h>, map_w> grid;
};
