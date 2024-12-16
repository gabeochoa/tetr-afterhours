

//
#include <iostream>

#include "rl.h"
//
#define AFTER_HOURS_ENTITY_HELPER
#define AFTER_HOURS_ENTITY_QUERY
#define AFTER_HOURS_SYSTEM
#include "afterhours/ah.h"
#define AFTER_HOURS_USE_RAYLIB
#include "afterhours/src/plugins/developer.h"
#include "afterhours/src/plugins/input_system.h"
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

namespace util {

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }
} // namespace util

//
const int map_h = 33;
const int map_w = 12;

const float keyReset = 0.05f;
const float dropReset = 0.20f;
const float rotateReset = 0.10f;

const float sz = 20;
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
void lock_entity(Entity &entity, const vec2 &pos,
                 const std::array<int, 16> &sh);

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

using afterhours::input::InputCollector;
//
#include "systems.h"
//

auto get_mapping() {
  std::map<InputAction, input::ValidInputs> mapping;
  mapping[InputAction::Left] = {
      raylib::KEY_LEFT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = -1,
      },
  };

  mapping[InputAction::Right] = {
      raylib::KEY_RIGHT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = 1,
      },
  };

  mapping[InputAction::Rotate] = {
      raylib::KEY_UP,                                       //
      raylib::GamepadButton::GAMEPAD_BUTTON_RIGHT_FACE_DOWN //
  };

  mapping[InputAction::Drop] = {
      raylib::KEY_DOWN,                                     //
      raylib::GamepadButton::GAMEPAD_BUTTON_RIGHT_FACE_LEFT //
  };

  mapping[InputAction::Drop] = {
      raylib::KEY_SPACE,                                  //
      raylib::GamepadButton::GAMEPAD_BUTTON_RIGHT_FACE_UP //
  };
  return mapping;
}

int main(void) {
  const int screenWidth = 720;
  const int screenHeight = 720;

  raylib::InitWindow(screenWidth, screenHeight, "tetr-afterhours");
  raylib::SetTargetFPS(200);

  // sophie
  {
    auto &entity = EntityHelper::createEntity();
    input::add_singleton_components<InputAction>(entity, get_mapping());
    entity.addComponent<NextPieceHolder>();
    entity.addComponent<Grid>();
  }

  SystemManager systems;

  // debug systems
  systems.register_update_system(
      std::make_unique<afterhours::developer::EnforceSingleton<Grid>>());
  input::enforce_singletons<InputAction>(systems);

  // external plugins
  systems.register_update_system(
      std::make_unique<afterhours::input::InputSystem<InputAction>>());
  // updates
  systems.register_update_system(std::make_unique<SpawnGround>());
  systems.register_update_system(std::make_unique<SpawnPieceIfNoneFalling>());
  systems.register_update_system(std::make_unique<ForceDrop>());
  systems.register_update_system(std::make_unique<Rotate>());
  systems.register_update_system(std::make_unique<Move>());
  systems.register_update_system(std::make_unique<Fall>());
  systems.register_update_system(std::make_unique<ClearLine>());
  // renders
  systems.register_render_system(std::make_unique<RenderGrid>());
  systems.register_render_system(std::make_unique<RenderPiece>());
  systems.register_render_system(std::make_unique<RenderGhost>());
  systems.register_render_system(std::make_unique<RenderPreview>());
  systems.register_render_system(
      std::make_unique<input::RenderConnectedGamepads>());

  while (!raylib::WindowShouldClose()) {
    raylib::BeginDrawing();
    {
      raylib::ClearBackground(color::BLACK_);
      systems.run(raylib::GetFrameTime());
      raylib::DrawFPS(500, 10);
    }
    raylib::EndDrawing();
  }

  raylib::CloseWindow();

  return 0;
}
