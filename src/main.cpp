
#include "rl.h"

#define AFTER_HOURS_INPUT_VALIDATION_ASSERT
#define AFTER_HOURS_ENTITY_HELPER
#define AFTER_HOURS_ENTITY_QUERY
#define AFTER_HOURS_SYSTEM
#define AFTER_HOURS_USE_RAYLIB
#include "afterhours/ah.h"
#include "afterhours/src/developer.h"
#include "afterhours/src/plugins/animation/timeline.h"
#include "afterhours/src/plugins/camera.h"
#include "afterhours/src/plugins/particles.h"
#include "afterhours/src/plugins/input_system.h"
#include "afterhours/src/plugins/window_manager.h"
#include <cassert>

//
#include "piece_data.h"
using namespace afterhours;

typedef raylib::Vector2 vec2;

constexpr float distance_sq(const vec2 a, const vec2 b) {
  return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

//
const int map_h = 33;
const int map_w = 12;

const float keyReset = 0.10f;
const float dropReset = 0.20f;
const float rotateReset = 0.10f;

const float sz = 20;
// Screen position of the board's top-left cell; applied by the camera.
const vec2 board_origin = {160, 40};
const float szm = 0.8f;

float TR = 0.25f;

std::vector<vec2> get_pips(const vec2 &pos, const std::array<int, 16> &sh) {
  std::vector<vec2> my_pips;
  for (size_t i = 0; i < 4; i++) {
    for (size_t j = 0; j < 4; j++) {
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

bool will_collide(EntityID id, vec2 pos, const std::array<int, 16> &shape);
struct PieceType;
void lock_entity(Entity &entity, const vec2 &pos, const PieceType &pt);

// These are not real header files, im just
// hijacking the include to paste the files here in this order
//
#include "colors.h"
//
#include "components.h"
//
#include "query.h"
//

enum class InputAction {
  None,
  Left,
  Right,
  Rotate,
  Down,
  Drop,
};

inline int to_int(InputAction action) {
  return static_cast<int>(action);
}

inline InputAction from_int(int value) {
  return static_cast<InputAction>(value);
}

using ::afterhours::input;
//
#include "systems.h"
//

auto get_mapping() {
  std::map<int, input::ValidInputs> mapping;
  mapping[to_int(InputAction::Left)] = {
      raylib::KEY_LEFT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = -1,
      },
  };

  mapping[to_int(InputAction::Right)] = {
      raylib::KEY_RIGHT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = 1,
      },
  };

  mapping[to_int(InputAction::Rotate)] = {
      raylib::KEY_UP,                                       //
      raylib::GamepadButton::GAMEPAD_BUTTON_RIGHT_FACE_DOWN //
  };

  mapping[to_int(InputAction::Down)] = {
      raylib::KEY_DOWN,                                     //
      raylib::GamepadButton::GAMEPAD_BUTTON_RIGHT_FACE_LEFT //
  };

  mapping[to_int(InputAction::Drop)] = {
      raylib::KEY_SPACE,                                  //
      raylib::GamepadButton::GAMEPAD_BUTTON_RIGHT_FACE_UP //
  };
  return mapping;
}

struct RenderFPS : System<window_manager::ProvidesCurrentResolution> {
  virtual void for_each_with(
      const Entity &,
      const window_manager::ProvidesCurrentResolution &pCurrentResolution,
      float) const override {
    raylib::DrawFPS((int)(pCurrentResolution.width() - 80), 0);
  }
};

void enforce_singletons(SystemManager &systems) {
  systems.register_update_system(
      std::make_unique<afterhours::developer::EnforceSingleton<Grid>>());
}

int main(void) {
  const int screenWidth = 720;
  const int screenHeight = 720;

  raylib::InitWindow(screenWidth, screenHeight, "tetr-afterhours");
  raylib::SetTargetFPS(200);

  // sophie
  {
    auto &entity = EntityHelper::createEntity();
    input::add_singleton_components(entity, get_mapping());
    window_manager::add_singleton_components(
        entity, window_manager::Resolution{screenWidth, screenHeight}, 200, {});
    entity.addComponent<NextPieceHolder>();
    entity.addComponent<Grid>();
    entity.addComponent<LineBurst>();
    entity.addComponent<ScreenShake>();
    camera::add_singleton_components(entity);
    entity.get<camera::HasCamera>().set_zoom(1.f);
    EntityHelper::registerSingleton<NextPieceHolder>(entity);
    EntityHelper::registerSingleton<Grid>(entity);
    EntityHelper::registerSingleton<ScreenShake>(entity);
  }

  for (int i = 0; i < map_w; i += 4) {
    auto &ground = EntityHelper::createEntity();
    ground.addComponent<Transform>(vec2{sz * (float)i, (map_h - 1) * sz});
    ground.addComponent<IsGround>();
    ground.addComponent<HasCollision>();
    ground.addComponent<PieceType>(0);
  }
  EntityHelper::merge_entity_arrays();

  SystemManager systems;

  // debug systems
  {
    enforce_singletons(systems);
    input::enforce_singletons(systems);
    window_manager::enforce_singletons(systems);
  }

  // external plugins
  { input::register_update_systems(systems); }

  // updates
  {
    systems.register_update_system(std::make_unique<SpawnPieceIfNoneFalling>());
    systems.register_update_system(std::make_unique<ForceDrop>());
    systems.register_update_system(std::make_unique<Rotate>());
    systems.register_update_system(std::make_unique<Move>());
    systems.register_update_system(std::make_unique<Fall>());
    systems.register_update_system(std::make_unique<ClearLine>());
    systems.register_update_system(std::make_unique<UpdateLineBurst>());
    systems.register_update_system(std::make_unique<ShakeCamera>());
  }

  // renders
  {
    systems.register_render_system(
        [](float) { raylib::ClearBackground(color::BACKGROUND); });
    camera::register_begin_camera(systems);
    systems.register_render_system(std::make_unique<RenderPanels>());
    systems.register_render_system(std::make_unique<RenderGrid>());
    systems.register_render_system(std::make_unique<RenderPiece>());
    systems.register_render_system(std::make_unique<RenderGhost>());
    systems.register_render_system(std::make_unique<RenderPreview>());
    systems.register_render_system(std::make_unique<RenderLineBurst>());
    camera::register_end_camera(systems);
    systems.register_render_system(std::make_unique<RenderFPS>());
  }

  while (!raylib::WindowShouldClose()) {
    raylib::BeginDrawing();
    { systems.run(raylib::GetFrameTime()); }
    raylib::EndDrawing();
  }

  raylib::CloseWindow();

  return 0;
}
