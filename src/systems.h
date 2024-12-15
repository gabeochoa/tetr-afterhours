
#pragma once

#include "components.h"
#include "raylib.h"
bool will_collide(EntityID id, vec2 pos, const std::array<int, 16> &shape) {
  auto pips = get_pips(pos, shape);
  for (auto &pip : pips) {
    if (pip.x < 0 || pip.x > (map_w - 1) * sz)
      return true;
  }

  return EQ()
      .whereNotID(id)
      .whereHasComponent<HasCollision>()
      .whereHasComponent<Transform>()
      .whereOverlaps(pos, shape)
      .has_values();
}

void lock_entity(Entity &entity, const vec2 &pos,
                 const std::array<int, 16> &sh) {
  entity.removeComponent<IsFalling>();
  entity.cleanup = true;

  const auto &pips = get_pips(pos, sh);
  for (auto &pip : pips) {
    auto &new_entity = EntityHelper::createEntity();
    new_entity.addComponent<Transform>(pip);
    new_entity.addComponent<IsLocked>();
    new_entity.addComponent<HasCollision>();
  }
}

struct InputSystem : System<InputCollector> {
  float DEADZONE = 0.25f;

  enum InputType {
    Keyboard,
    Gamepad,
    GamepadWithAxis,
  };

  using KeyCode = int;
  struct GamepadAxisWithDir {
    raylib::GamepadAxis axis;
    float dir = -1;
  };
  using AnyInput =
      std::variant<KeyCode, GamepadAxisWithDir, raylib::GamepadButton>;
  using ValidInputs = std::vector<AnyInput>;
  //
  std::map<InputCollector::InputAction, ValidInputs> mapping;
  //

  InputSystem() {
    mapping[InputCollector::InputAction::Left] = {
        raylib::KEY_LEFT,
        GamepadAxisWithDir{
            .axis = raylib::GAMEPAD_AXIS_LEFT_X,
            .dir = -1,
        },
    };

    mapping[InputCollector::InputAction::Right] = {
        raylib::KEY_RIGHT,
        GamepadAxisWithDir{
            .axis = raylib::GAMEPAD_AXIS_LEFT_X,
            .dir = 1,
        },
    };

    mapping[InputCollector::InputAction::Rotate] = {
        raylib::KEY_UP,                                       //
        raylib::GamepadButton::GAMEPAD_BUTTON_RIGHT_FACE_DOWN //
    };

    mapping[InputCollector::InputAction::Drop] = {
        raylib::KEY_DOWN,                                     //
        raylib::GamepadButton::GAMEPAD_BUTTON_RIGHT_FACE_LEFT //
    };

    mapping[InputCollector::InputAction::Drop] = {
        raylib::KEY_SPACE,                                  //
        raylib::GamepadButton::GAMEPAD_BUTTON_RIGHT_FACE_UP //
    };
  }

  float visit_key(int keycode) {
    return raylib::IsKeyPressed(keycode) ? 1.f : 0.f;
  }

  float visit_key_down(int keycode) {
    return raylib::IsKeyDown(keycode) ? 1.f : 0.f;
  }

  float visit_axis(GamepadAxisWithDir axis_with_dir) {
    // Note: this one is a bit more complex because we have to check if you
    // are pushing in the right direction while also checking the magnitude
    float mvt = raylib::GetGamepadAxisMovement(0, axis_with_dir.axis);
    // Note: The 0.25 is how big the deadzone is
    // TODO consider making the deadzone configurable?
    if (util::sgn(mvt) == axis_with_dir.dir && abs(mvt) > DEADZONE) {
      return abs(mvt);
    }
    return 0.f;
  }

  float visit_button(raylib::GamepadButton button) {
    return raylib::IsGamepadButtonPressed(0, button) ? 1.f : 0.f;
  }

  float visit_button_down(raylib::GamepadButton button) {
    return raylib::IsGamepadButtonDown(0, button) ? 1.f : 0.f;
  }

  float check_single_action(ValidInputs valid_inputs) {
    float value = 0.f;
    for (auto &input : valid_inputs) {
      value =
          fmax(value,      //
               std::visit( //
                   util::overloaded{
                       //
                       [this](int keycode) { return visit_key_down(keycode); },
                       [this](GamepadAxisWithDir axis_with_dir) {
                         return visit_axis(axis_with_dir);
                       },
                       [this](raylib::GamepadButton button) {
                         return visit_button_down(button);
                       },
                       [](auto) {}},
                   input));
    }
    return value;
  }

  virtual void for_each_with(Entity &, InputCollector &collector,
                             float dt) override {
    collector.inputs.clear();

    for (auto &kv : mapping) {
      InputCollector::InputAction action = kv.first;
      ValidInputs vis = kv.second;
      float amount = check_single_action(vis);
      if (amount > 0.f) {
        collector.inputs.push_back(InputCollector::InputActionDone{
            .action = action, .amount_pressed = 1.f, .length_pressed = dt});
      }
    }
  }
};

struct ForceDrop : System<Transform, IsFalling, PieceType> {
  float timer;
  float timerReset;
  ForceDrop() : timer(dropReset), timerReset(dropReset) {}
  virtual ~ForceDrop() {}

  bool is_space = false;

  virtual bool should_run(float dt) override {
    if (timer < 0) {
      timer = timerReset;

      return is_space;
    }
    timer -= dt;

    OptEntity opt_collector =
        EQ().whereHasComponent<InputCollector>().gen_first();
    Entity &collector = opt_collector.asE();
    InputCollector &inp = collector.get<InputCollector>();

    is_space = false;

    for (auto &actions_done : inp.inputs) {
      switch (actions_done.action) {
      case InputCollector::InputAction::Drop:
        is_space = actions_done.amount_pressed > 0.f;
        break;
      default:
        break;
      }
    }

    return false;
  }

  virtual void for_each_with(Entity &entity, Transform &transform, IsFalling &,
                             PieceType &pt, float) override {
    if (!is_space)
      return;
    vec2 p = transform.pos();
    vec2 offset = vec2{0, sz};
    while (!will_collide(entity.id, p + offset, pt.shape)) {
      p += offset;
    }
    transform.update(p);
    lock_entity(entity, p, pt.shape);
  }
};

struct Move : System<Transform, IsFalling, PieceType> {
  float timer;
  float timerReset;

  bool is_left_pressed;
  bool is_right_pressed;
  bool is_down_pressed;

  Move() : timer(keyReset), timerReset(keyReset) {}
  virtual ~Move() {}

  virtual bool should_run(float dt) override {

    if (timer < 0) {
      timer = timerReset;

      return true;
    }
    timer -= dt;

    OptEntity opt_collector =
        EQ().whereHasComponent<InputCollector>().gen_first();
    Entity &collector = opt_collector.asE();
    InputCollector &inp = collector.get<InputCollector>();

    is_left_pressed = false;
    is_right_pressed = false;
    is_down_pressed = false;

    // TODO do we need to eat these?
    for (auto &actions_done : inp.inputs) {
      switch (actions_done.action) {
      case InputCollector::InputAction::Left:
        is_left_pressed = actions_done.amount_pressed > 0.f;
        break;
      case InputCollector::InputAction::Right:
        is_right_pressed = actions_done.amount_pressed > 0.f;
        break;
      case InputCollector::InputAction::Down:
        is_down_pressed = actions_done.amount_pressed > 0.f;
        break;
      default:
        break;
      }
    }
    return false;
  }

  virtual void for_each_with(Entity &entity, Transform &transform, IsFalling &,
                             PieceType &pt, float) override {

    vec2 p = transform.pos();
    if (is_left_pressed)
      p -= vec2{sz, 0};
    if (is_right_pressed)
      p += vec2{sz, 0};
    if (is_down_pressed)
      p += vec2{0, sz};

    if (will_collide(entity.id, p, pt.shape)) {
      return;
    }
    transform.update(p);
  }
};

struct Rotate : System<Transform, IsFalling, PieceType> {
  float timer;
  float timerReset;

  bool is_up_pressed;

  Rotate() : timer(rotateReset), timerReset(rotateReset) {}
  virtual ~Rotate() {}

  virtual bool should_run(float dt) override {
    if (timer < 0) {
      timer = timerReset;
      return true;
    }
    timer -= dt;
    is_up_pressed = false;

    OptEntity opt_collector =
        EQ().whereHasComponent<InputCollector>().gen_first();
    Entity &collector = opt_collector.asE();
    InputCollector &inp = collector.get<InputCollector>();

    for (auto &actions_done : inp.inputs) {
      switch (actions_done.action) {
      case InputCollector::InputAction::Rotate:
        is_up_pressed = actions_done.amount_pressed > 0.f;
        break;
      default:
        break;
      }
    }
    return false;
  }

  virtual void for_each_with(Entity &entity, Transform &transform, IsFalling &,
                             PieceType &pt, float) override {
    if (!is_up_pressed) {
      return;
    }

    vec2 pos = transform.pos();
    auto new_angle = (pt.angle + 1) % 4;
    auto new_shape = type_to_rotated_array(pt.type, new_angle);

    // no collision?
    if (!will_collide(entity.id, pos, new_shape)) {
      pt.angle = new_angle;
      pt.shape = new_shape;
      return;
    }

    // rotation didnt fit,
    // wall kick

    std::array<std::array<std::pair<int, int>, 4>, 4> tests =
        pt.type == 0 ? long_boi_tests : wall_kick_tests;

    for (auto pair : tests[(size_t)new_angle]) {
      vec2 offset = vec2{pair.first * sz, pair.second * sz};
      if (will_collide(entity.id, pos + offset, new_shape))
        continue;
      pt.angle = new_angle;
      pt.shape = new_shape;
      transform.update(pos + offset);
      return;
    }
  }
};

struct Fall : System<Transform, IsFalling, PieceType> {
  float timer;
  float timerReset;
  Fall() : timer(TR), timerReset(TR) {}

  virtual ~Fall() {}

  virtual bool should_run(float dt) override {
    if (timer < 0) {
      timer = timerReset;
      return true;
    }
    timer -= dt;
    return false;
  }

  virtual void for_each_with(Entity &entity, Transform &transform, IsFalling &,
                             PieceType &pt, float) override {
    auto p = transform.pos() + vec2{0, sz};
    if (will_collide(entity.id, p, pt.shape)) {
      lock_entity(entity, transform.pos(), pt.shape);
      return;
    }
    //
    //
    transform.update(p);
  }
};

struct RenderGrid : System<> {
  virtual ~RenderGrid() {}
  virtual void once(float) {
    vec2 full_size = {sz, sz};
    vec2 size = full_size * szm;
    for (int i = 0; i < map_w; i++) {
      for (int j = 0; j < map_h; j++) {
        raylib::DrawRectangleV({(i * sz), (j * sz)}, size, raylib::GRAY);
      }
    }
  }
};

struct RenderLocked : System<Transform, IsLocked> {
  virtual ~RenderLocked() {}
  virtual void for_each_with(const Entity &, const Transform &transform,
                             const IsLocked &, float) const override {

    raylib::Color col = color::BLACK;
    raylib::DrawRectangleV(transform.pos(), {sz * szm, sz * szm}, col);
  }
};

struct RenderPiece : System<Transform, PieceType> {
  virtual ~RenderPiece() {}
  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             const PieceType &pieceType, float) const override {

    raylib::Color col = entity.has<IsGround>()
                            ? color::BLACK_
                            : color::piece_color(pieceType.type);

    for (size_t i = 0; i < 4; i++) {
      for (size_t j = 0; j < 4; j++) {
        if (pieceType.shape[j * 4 + i] == 0)
          continue;
        raylib::DrawRectangleV(
            {transform.pos().x + (i * sz), transform.pos().y + (j * sz)},
            {sz * szm, sz * szm},

            col);
      }
    }
  }
};

struct RenderGhost : System<Transform, IsFalling, PieceType> {
  virtual ~RenderGhost() {}
  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             const IsFalling &, const PieceType &pt,
                             float) const override {
    vec2 p = transform.pos();
    vec2 offset = vec2{0, sz};
    while (!will_collide(entity.id, p + offset, pt.shape)) {
      if (p.y > map_h * sz)
        break;
      p += offset;
    }

    raylib::Color color = color::piece_color(pt.type);
    color.a = 100;

    for (size_t i = 0; i < 4; i++) {
      for (size_t j = 0; j < 4; j++) {
        if (pt.shape[j * 4 + i] == 0)
          continue;
        raylib::DrawRectangleV({p.x + (i * sz), p.y + (j * sz)},
                               {sz * szm, sz * szm}, color);
      }
    }
  }
};

struct SpawnPieceIfNoneFalling : System<> {
  virtual ~SpawnPieceIfNoneFalling() {}

  virtual bool should_run(float) {
    return !EQ().whereHasComponent<IsFalling>().has_values();
  }

  virtual void once(float) {
    auto &entity = EntityHelper::createEntity();
    entity.addComponent<Transform>(vec2{20, 20});
    entity.addComponent<IsFalling>();
    entity.addComponent<HasCollision>();
    entity.addComponent<PieceType>(rand() % 6);

    std::cout << "spawned piece of type " << entity.get<PieceType>().type
              << std::endl;
  };
};

struct SpawnGround : System<> {
  bool init = false;
  virtual ~SpawnGround() {}

  virtual bool should_run(float) {
    if (!init) {
      init = true;
      for (int i = 0; i < map_w; i += 4) {
        auto &entity = EntityHelper::createEntity();
        entity.addComponent<Transform>(vec2{20.f * i, (map_h - 1) * 20.f});
        entity.addComponent<IsGround>();
        entity.addComponent<HasCollision>();
        entity.addComponent<PieceType>(0);
      }
    }
    return false;
  }
};
