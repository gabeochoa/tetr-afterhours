

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
#include "colors.h"
struct Transform : public BaseComponent {
  Transform(vec2 pos) : position(pos) {}

  virtual ~Transform() {}

  [[nodiscard]] vec2 pos() const { return position; }
  void update(vec2 v) { position = v; }

private:
  vec2 position;
};

struct IsFalling : public BaseComponent {};
struct PieceType : public BaseComponent {
  PieceType(int t) : type(t) {}
  int type;
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
};

struct Move : System<Transform, IsFalling> {
  float timer;
  float timerReset;
  Move() : timer(0.5f), timerReset(0.5f) {}

  virtual ~Move() {}

  virtual bool should_run(float dt) override {
    if (timer < 0) {
      timer = timerReset;
      return true;
    }
    timer -= dt;
    return false;
  }

  virtual void for_each_with(Entity &, Transform &transform, IsFalling &,
                             float) override {
    auto p = transform.pos() + vec2{0, 20.f};
    //
    //
    transform.update(p);
  }
};

struct RenderGrid : System<> {
  virtual ~RenderGrid() {}
  virtual void once(float) {
    float sz = 20;
    vec2 full_size = {sz, sz};
    vec2 size = full_size * 0.8f;
    int map_h = 33;
    int map_w = 12;
    for (int i = 0; i < map_w; i++) {
      for (int j = 0; j < map_h; j++) {
        raylib::DrawRectangleV({(i * sz) + 2, (j * sz) + 2}, size,
                               raylib::RAYWHITE);
      }
    }
  }
};

struct RenderPiece : System<Transform, PieceType> {
  virtual ~RenderPiece() {}
  virtual void for_each_with(const Entity &, const Transform &transform,
                             const PieceType &pieceType, float) const override {
    raylib::DrawRectangleV(transform.pos(), {20, 20},
                           color::piece_color(pieceType.type));
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
    entity.addComponent<PieceType>(rand() % 6);
  };
};

int main(void) {
  const int screenWidth = 720;
  const int screenHeight = 720;

  raylib::InitWindow(screenWidth, screenHeight, "tetr-afterhours");
  raylib::SetTargetFPS(60);

  SystemManager systems;
  systems.register_update_system(std::make_unique<SpawnPieceIfNoneFalling>());
  systems.register_update_system(std::make_unique<Move>());
  //
  systems.register_render_system(std::make_unique<RenderGrid>());
  systems.register_render_system(std::make_unique<RenderPiece>());

  while (!raylib::WindowShouldClose()) {

    raylib::BeginDrawing();
    {
      raylib::ClearBackground(raylib::GRAY);
      systems.run(raylib::GetFrameTime());
    }
    raylib::EndDrawing();
  }

  raylib::CloseWindow();

  return 0;
}
