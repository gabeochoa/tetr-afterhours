

//
#include <iostream>

#define AFTER_HOURS_ENTITY_HELPER
#define AFTER_HOURS_ENTITY_QUERY
#define AFTER_HOURS_SYSTEM
#include "afterhours/ah.h"

#include "rl.h"
#include <cassert>

//
#include "piece_data.h"
using namespace afterhours;

typedef raylib::Vector2 vec2;
typedef raylib::Vector3 vec3;
typedef raylib::Vector4 vec4;

constexpr float distance_sq(const vec2 a, const vec2 b) {
  return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

//
int map_h = 33;
int map_w = 12;
float TR = 0.25f;
float keyReset = 0.15f;

float sz = 20;
float szm = 0.8f;

std::vector<vec2> get_pips(const vec2 &pos, const std::array<int, 16> &sh) {
  std::vector<vec2> my_pips;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (sh[j * 4 + i] == 0)
        continue;
      my_pips.push_back({
          pos.x + (i * sz),
          pos.y + (j * sz),
      });
    }
  }
  return my_pips;
}

//
#include "colors.h"
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

struct EQ : public EntityQuery<EQ> {
  struct WhereInRange : EntityQuery::Modification {
    vec2 position;
    float range;

    // TODO mess around with the right epsilon here
    explicit WhereInRange(vec2 pos, float r = 0.01f)
        : position(pos), range(r) {}
    bool operator()(const Entity &entity) const override {
      vec2 pos = entity.get<Transform>().pos();
      return distance_sq(position, pos) < (range * range);
    }
  };

  EQ &whereInRange(const vec2 &position, float range) {
    return add_mod(new WhereInRange(position, range));
  }

  EQ &orderByDist(const vec2 &position) {
    return orderByLambda([position](const Entity &a, const Entity &b) {
      float a_dist = distance_sq(a.get<Transform>().pos(), position);
      float b_dist = distance_sq(b.get<Transform>().pos(), position);
      return a_dist < b_dist;
    });
  }

  struct WhereOverlaps : EntityQuery::Modification {
    vec2 position;
    std::array<int, 16> shape;
    std::vector<vec2> pips;

    explicit WhereOverlaps(vec2 pos, std::array<int, 16> s)
        : position(pos), shape(s) {
      pips = get_pips(pos, s);
    }

    bool operator()(const Entity &entity) const override {
      auto mypos = entity.get<Transform>().pos();
      if (entity.is_missing<PieceType>()) {
        for (auto &p : pips) {
          float a_dist = distance_sq(mypos, p);
          if (a_dist < sz)
            return true;
        }
        return false;
      }

      auto mypips = get_pips(mypos, entity.get<PieceType>().shape);
      for (auto &mypip : mypips) {
        for (auto &p : pips) {
          float a_dist = distance_sq(mypip, p);
          if (a_dist < sz)
            return true;
        }
      }
      return false;
    }
  };

  EQ &whereOverlaps(const vec2 &position, std::array<int, 16> shape) {
    return add_mod(new WhereOverlaps(position, shape));
  }
};

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

struct ForceDrop : System<Transform, IsFalling, PieceType> {
  ForceDrop() {}
  virtual ~ForceDrop() {}

  bool is_space = false;

  void once(float) override {
    is_space = raylib::IsKeyPressed(raylib::KEY_SPACE);
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
    is_left_pressed = raylib::IsKeyDown(raylib::KEY_LEFT);
    is_right_pressed = raylib::IsKeyDown(raylib::KEY_RIGHT);
    is_down_pressed = raylib::IsKeyDown(raylib::KEY_DOWN);
    return false;
  }

  virtual void for_each_with(Entity &entity, Transform &transform, IsFalling &,
                             PieceType &pt, float) override {
    bool is_space_pressed = raylib::IsKeyDown(raylib::KEY_SPACE);

    vec2 p = transform.pos();
    if (is_left_pressed)
      p -= vec2{sz, 0};
    if (is_right_pressed)
      p += vec2{sz, 0};
    if (is_down_pressed)
      p += vec2{0, sz};

    if (is_space_pressed)
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

  Rotate() : timer(keyReset), timerReset(keyReset) {}
  virtual ~Rotate() {}

  virtual bool should_run(float dt) override {
    if (timer < 0) {
      timer = timerReset;
      return true;
    }
    timer -= dt;
    is_up_pressed = raylib::IsKeyDown(raylib::KEY_UP);
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

    for (auto pair : tests[new_angle]) {
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

    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
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

    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
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

int main(void) {
  const int screenWidth = 720;
  const int screenHeight = 720;

  raylib::InitWindow(screenWidth, screenHeight, "tetr-afterhours");
  raylib::SetTargetFPS(60);

  for (int i = 0; i < map_w; i += 4) {
    auto &entity = EntityHelper::createEntity();
    entity.addComponent<Transform>(vec2{20.f * i, (map_h - 1) * 20.f});
    entity.addComponent<IsGround>();
    entity.addComponent<HasCollision>();
    entity.addComponent<PieceType>(0);
  }

  SystemManager systems;
  systems.register_update_system(std::make_unique<SpawnPieceIfNoneFalling>());
  systems.register_update_system(std::make_unique<ForceDrop>());
  systems.register_update_system(std::make_unique<Rotate>());
  systems.register_update_system(std::make_unique<Move>());
  systems.register_update_system(std::make_unique<Fall>());
  //
  systems.register_render_system(std::make_unique<RenderGrid>());
  systems.register_render_system(std::make_unique<RenderPiece>());
  systems.register_render_system(std::make_unique<RenderGhost>());
  systems.register_render_system(std::make_unique<RenderLocked>());

  while (!raylib::WindowShouldClose()) {
    raylib::BeginDrawing();
    {
      raylib::ClearBackground(color::BLACK_);
      systems.run(raylib::GetFrameTime());
    }
    raylib::EndDrawing();
  }

  raylib::CloseWindow();

  return 0;
}
