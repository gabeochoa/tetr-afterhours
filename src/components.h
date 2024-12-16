
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
struct IsLocked : public BaseComponent {};
struct IsFalling : public BaseComponent {};

struct PieceType : public BaseComponent {
  PieceType(int t) : type(t), angle(0) {
    shape = type_to_rotated_array(type, angle);
  }
  int type;
  std::array<int, 16> shape;
  int angle;
};

struct InputCollector : public BaseComponent {
  enum class InputAction {
    None,
    Left,
    Right,
    Rotate,
    Down,
    Drop,
  };
  struct InputActionDone {
    InputAction action;
    float amount_pressed;
    float length_pressed;
  };
  std::vector<InputActionDone> inputs;
  float since_last_input = 0.f;
};

struct NextPieceHolder : public BaseComponent {
  int next_type;
  NextPieceHolder() : next_type(rand() % 6) {}
};

struct Grid : public BaseComponent {
  std::array<std::array<int, map_h>, map_w> grid;
};
