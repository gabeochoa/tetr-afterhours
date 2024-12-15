#include <iostream>

#define AFTER_HOURS_ENTITY_HELPER
#define AFTER_HOURS_ENTITY_QUERY
#define AFTER_HOURS_SYSTEM
#include "afterhours/ah.h"

#include "colors.h"
#include "piece_data.h"
#include "rl.h"
#include <cassert>

using namespace afterhours;

typedef raylib::Vector2 vec2;
typedef raylib::Vector3 vec3;
typedef raylib::Vector4 vec4;

struct Transform : public BaseComponent {
  Transform(vec2 pos) : position(pos) {}

  virtual ~Transform() {}

  [[nodiscard]] vec2 pos() const { return position; }
  void update(vec2 v) { position = v; }

private:
  vec2 position;
};

struct IsFalling : public BaseComponent {};

struct Move : System<Transform, IsFalling> {
  virtual ~Move() {}
  virtual void for_each_with(Entity &, Transform &transform, IsFalling &,
                             float dt) override {
    auto p = transform.pos() + vec2{0, 100.f * dt};
    transform.update(p);
    // std::cout << p << std::endl;
  }
};

struct RenderPiece : System<Transform> {
  virtual ~RenderPiece() {}
  virtual void for_each_with(const Entity &, const Transform &transform,
                             float) const override {
    // TODO color
    raylib::DrawRectangleV(transform.pos(), {20, 20}, raylib::BLUE);
  }
};

int main(void) {
  const int screenWidth = 800;
  const int screenHeight = 450;

  raylib::InitWindow(screenWidth, screenHeight, "tetr-afterhours");
  raylib::SetTargetFPS(60);

  SystemManager systems;
  systems.register_update_system(std::make_unique<Move>());
  systems.register_render_system(std::make_unique<RenderPiece>());

  auto &entity = EntityHelper::createEntity();
  entity.addComponent<Transform>(vec2{10, 10});
  entity.addComponent<IsFalling>();

  while (!raylib::WindowShouldClose()) {

    raylib::BeginDrawing();
    {

      raylib::ClearBackground(raylib::RAYWHITE);
      raylib::DrawText("Congrats! You created your first window!", 190, 200, 20,
                       raylib::LIGHTGRAY);

      systems.run(raylib::GetFrameTime());
    }
    raylib::EndDrawing();
  }

  raylib::CloseWindow();

  return 0;
}
