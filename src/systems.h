
#pragma once

#include "colors.h"
#include "components.h"
#include "piece_data.h"

bool will_collide(EntityID id, vec2 pos, const std::array<int, 16> &shape) {
  Grid &gridC = *EntityHelper::get_singleton_cmp<Grid>();

  auto pips = get_pips(pos, shape);
  for (auto &pip : pips) {
    // check inside map
    if (pip.x < 0 || pip.x > (map_w - 1) * sz || pip.y < 0 ||
        pip.y > (map_h - 1) * sz)
      return true;
    // check if locked
    if (gridC.grid[(size_t)(pip.x / sz)][(size_t)(pip.y / sz)] > 0)
      return true;
  }

  // check ground
  return EQ()
      .whereNotID(id)
      .whereHasComponent<HasCollision>()
      .whereHasComponent<Transform>()
      .whereOverlaps(pos, shape)
      .has_values();
}

void lock_entity(Entity &entity, const vec2 &pos, const PieceType &pt) {
  entity.removeComponent<IsFalling>();
  entity.cleanup = true;

  Grid &gridC = *EntityHelper::get_singleton_cmp<Grid>();
  // Cells store type + 1 so locked blocks keep their colour; 0 is empty.
  for (auto &pip : get_pips(pos, pt.shape)) {
    gridC.grid[(size_t)(pip.x / sz)][(size_t)(pip.y / sz)] = pt.type + 1;
  }
}

void draw_cell(vec2 pos, raylib::Color color) {
  raylib::DrawRectangleRounded({pos.x, pos.y, sz * szm, sz * szm}, 0.3f, 4,
                               color);
}

void draw_shape(vec2 pos, const std::array<int, 16> &shape,
                raylib::Color color) {
  for (auto &pip : get_pips(pos, shape))
    draw_cell(pip, color);
}

// Actions held this frame (any device). Empty before the input collector
// exists.
std::set<InputAction> held_actions() {
  std::set<InputAction> held;
  input::PossibleInputCollector inpc = input::get_input_collector();
  if (!inpc.has_value())
    return held;
  for (auto &actions_done : inpc.inputs())
    if (actions_done.amount_pressed > 0.f)
      held.insert(from_int(actions_done.action));
  return held;
}

// Single repeat-timing type (CORRECT-E1). Decrement-then-test, inputs are
// sampled by the caller BEFORE tick() so for_each_with never acts on last
// call's input. Period is mutable so Fall can follow the global TR speed-up.
struct RepeatGate {
  float period; float t;
  explicit RepeatGate(float p): period(p), t(p) {}
  bool tick(float dt){ t-=dt; if(t<=0.f){ t=period; return true; } return false; }
};

struct ForceDrop : System<Transform, IsFalling, PieceType> {
  RepeatGate gate{dropReset};
  ForceDrop() { gate.t = 0; }

  virtual bool should_run(float dt) override {
    bool is_space = held_actions().contains(InputAction::Drop);
    bool ready = gate.tick(dt);
    return is_space && ready;
  }

  virtual void for_each_with(Entity &entity, Transform &transform, IsFalling &,
                             PieceType &pt, float) override {
    vec2 p = transform.pos();
    vec2 offset = vec2{0, sz};
    while (!will_collide(entity.id, p + offset, pt.shape)) {
      p += offset;
    }
    transform.update(p);
    lock_entity(entity, p, pt);
    EntityHelper::get_singleton_cmp<ScreenShake>()->t = 0;
  }
};

struct Move : System<Transform, IsFalling, PieceType> {
  RepeatGate gate{keyReset};
  std::set<InputAction> held;

  virtual bool should_run(float dt) override {
    held = held_actions();
    return gate.tick(dt);
  }

  virtual void for_each_with(Entity &entity, Transform &transform, IsFalling &,
                             PieceType &pt, float) override {

    vec2 p = transform.pos();
    if (held.contains(InputAction::Left))
      p -= vec2{sz, 0};
    if (held.contains(InputAction::Right))
      p += vec2{sz, 0};
    if (held.contains(InputAction::Down))
      p += vec2{0, sz};

    if (will_collide(entity.id, p, pt.shape)) {
      return;
    }
    transform.update(p);
  }
};

struct Rotate : System<Transform, IsFalling, PieceType> {
  RepeatGate gate{rotateReset};
  bool is_up_pressed = false;

  virtual bool should_run(float dt) override {
    is_up_pressed = held_actions().contains(InputAction::Rotate);
    return gate.tick(dt);
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
  RepeatGate gate{TR};

  virtual bool should_run(float dt) override {
    gate.period = TR; // ClearLine speeds the game up by lowering TR
    return gate.tick(dt);
  }

  virtual void for_each_with(Entity &entity, Transform &transform, IsFalling &,
                             PieceType &pt, float) override {
    auto p = transform.pos() + vec2{0, sz};
    if (will_collide(entity.id, p, pt.shape)) {

      // In the situation where it will collide but you could rotate and keep
      // going, lets wait a bit if the user is trying to rotate

      input::PossibleInputCollector inpc =
          input::get_input_collector();
      if (inpc.has_value() && inpc.since_last_input() > 1.f) {
        lock_entity(entity, transform.pos(), pt);
      }

      return;
    }
    transform.update(p);
  }
};

struct ClearLine : System<Grid, LineBurst> {
  virtual void for_each_with(Entity &, Grid &gridC, LineBurst &burst,
                             float) override {

    auto &grid = gridC.grid;

    for (size_t j = 0; j < map_h; j++) {

      int sum = 0;
      for (size_t i = 0; i < map_w; i++)
        if (grid[i][j] > 0)
          sum++;

      if (sum != map_w)
        continue;

      // clear row()
      for (size_t i = 0; i < map_w; i++) {
        grid[i][j] = 0;
        for (int n = 0; n < 4; n++) {
          auto &p = burst.emitter.spawn();
          p.pos = {(float)i * sz + sz / 2, (float)j * sz + sz / 2};
          p.vel = {(float)(rand() % 300 - 150), (float)(-100 - rand() % 250)};
          p.size = (float)(4 + rand() % 4);
          p.life = 0.8f + (float)(rand() % 40) / 100.f;
          p.color = color::piece_color(rand() % 7);
        }
      }

      // move everything above down
      for (size_t k = j; k > 0; k--) {
        for (size_t i = 0; i < map_w; i++)
          grid[i][k] = grid[i][k - 1];
      }

      // replace top row with empty tiles
      for (size_t i = 0; i < map_w; i++)
        grid[i][0] = 0;

      // Increment num lines and speed up game
      gridC.totalCleared++;

      // speed up
      TR -= 0.1f;
    }
  }
};

struct RenderGrid : System<Grid> {
  virtual void for_each_with(const Entity &, const Grid &gridC,
                             float) const override {
    // The bottom row holds the invisible ground pieces, so skip it.
    for (size_t i = 0; i < map_w; i++) {
      for (size_t j = 0; j < map_h - 1; j++) {
        int val = gridC.grid[i][j];
        draw_cell({(float)i * sz, (float)j * sz},
                  val == 0 ? color::EMPTY_CELL : color::piece_color(val - 1));
      }
    }
  }
};

struct RenderPiece : System<Transform, PieceType> {
  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             const PieceType &pieceType, float) const override {
    if (entity.has<IsGround>())
      return;
    draw_shape(transform.pos(), pieceType.shape,
               color::piece_color(pieceType.type));
  }
};

const float pad = 8;
const float gap = sz * (1 - szm); // space after the last cell
const float side_x = map_w * sz + 2 * pad + 12;

struct RenderPanels : System<> {
  virtual void once(float) const override {
    raylib::DrawRectangleRounded(
        {-pad, -pad, map_w * sz - gap + 2 * pad,
         (map_h - 1) * sz - gap + 2 * pad},
        0.03f, 6,
        color::PANEL);
    raylib::DrawRectangleRounded({side_x, -pad, 120, 200}, 0.1f, 6,
                                 color::PANEL);
  }
};

struct RenderPreview : System<NextPieceHolder, Grid> {
  virtual void for_each_with(const Entity &, const NextPieceHolder &nph,
                             const Grid &gridC, float) const override {
    const int x = (int)(side_x + 16);
    raylib::DrawText("NEXT", x, 8, 16, color::LABEL);
    draw_shape({side_x + 20, 36}, type_to_rotated_array(nph.next_type, 0),
               color::piece_color(nph.next_type));
    raylib::DrawText("LINES", x, 124, 16, color::LABEL);
    raylib::DrawText(std::to_string(gridC.totalCleared).c_str(), x, 146, 28,
                     raylib::RAYWHITE);
  }
};

struct RenderGhost : System<Transform, IsFalling, PieceType> {
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
    draw_shape(p, pt.shape, color);
  }
};

struct SpawnPieceIfNoneFalling : System<NextPieceHolder> {
  virtual bool should_run(float) override {
    return !EQ().whereHasComponent<IsFalling>().has_values();
  }

  virtual void for_each_with(Entity &, NextPieceHolder &nph, float) override {

    auto &entity = EntityHelper::createEntity();
    entity.addComponent<Transform>(vec2{20, 20});
    entity.addComponent<IsFalling>();
    entity.addComponent<HasCollision>();
    entity.addComponent<PieceType>(nph.next_type);

    nph.next_type = (rand() % 6);

    std::cout << "spawned piece of type " << entity.get<PieceType>().type
              << std::endl;
  }
};

struct UpdateLineBurst : System<LineBurst> {
  virtual void for_each_with(Entity &, LineBurst &burst, float dt) override {
    burst.emitter.update(dt);
  }
};

struct RenderLineBurst : System<LineBurst> {
  virtual void for_each_with(const Entity &, const LineBurst &burst,
                             float) const override {
    burst.emitter.each([](const particles::Particle &p) {
      raylib::Color c = p.color;
      c.a = (unsigned char)(255.f * (1.f - p.progress()));
      raylib::DrawRectangleV({p.pos.x - p.size / 2, p.pos.y - p.size / 2},
                             {p.size, p.size}, c);
    });
  }
};

struct ShakeCamera : System<camera::HasCamera, ScreenShake> {
  virtual void for_each_with(Entity &, camera::HasCamera &cam,
                             ScreenShake &shake, float dt) override {
    // Same keys as afterhours presets::shake(); that header needs the UI.
    static const motion::Timeline thud{
        .keys = {{0.f, 0.f}, {0.08f, 1.f}, {0.16f, -1.f}, {0.22f, 0.667f},
                 {0.28f, 0.f}},
        .curve = motion::curves::ease_out_quad};
    shake.t += dt;
    cam.set_offset({board_origin.x, board_origin.y + 6.f * thud.at(shake.t)});
  }
};
