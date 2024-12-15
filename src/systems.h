
#pragma once

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

  Rotate() : timer(rotateReset), timerReset(rotateReset) {}
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
