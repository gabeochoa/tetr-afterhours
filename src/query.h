
#pragma once
// again not a real headerfile

struct EQ : public EntityQuery<EQ> {
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
