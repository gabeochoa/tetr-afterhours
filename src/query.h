
#pragma once
// again not a real headerfile

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
