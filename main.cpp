#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ios>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using U8 = uint8_t;
using U32 = uint32_t;
using U64 = uint64_t;

using S32 = int32_t;

using F32 = float;
using F64 = double;

template <typename T> T read() {
  T t;
  std::cin >> t;
  return t;
}

struct Cell {
  std::optional<U8> ore;
  bool hasHole = false;
};

class Map {
  std::vector<std::vector<Cell>> cells;

public:
  Map(U32 width, U32 height) : cells(height, std::vector<Cell>(width)) {}

  [[nodiscard]] U32 getHeight() const { return cells.size(); }

  [[nodiscard]] U32 getWidth() const { return cells.front().size(); }

  [[nodiscard]] bool hasOreCount(U32 i, U32 j) const {
    return cells[i][j].ore.has_value();
  }

  [[nodiscard]] U32 getOreCount(U32 i, U32 j) const {
    return cells[i][j].ore.value();
  }

  void readUpdate() {
    for (U64 i = 0; i < cells.size(); i++) {
      for (U64 j = 0; j < cells.front().size(); j++) {
        const auto oreString = read<std::string>();
        if (oreString == "?") {
          cells[i][j].ore = std::nullopt;
        } else {
          cells[i][j].ore = std::stoul(oreString);
        }
        cells[i][j].hasHole = read<U8>() == 1;
      }
    }
  }
};

enum class EntityType { None, MyRobot, TheirRobot, Radar, Trap };

EntityType readEntityType() {
  switch (read<U32>()) {
  case 0:
    return EntityType::MyRobot;
  case 1:
    return EntityType::TheirRobot;
  case 2:
    return EntityType::Radar;
  case 3:
    return EntityType::Trap;
  }
  throw std::runtime_error("Not a valid unit type.");
}

struct Position {
  U32 x = 0;
  U32 y = 0;

  Position() = default;
  Position(U32 x, U32 y) : x(x), y(y) {}

  bool operator==(const Position &rhs) const {
    return x == rhs.x && y == rhs.y;
  }

  [[nodiscard]] U32 distanceTo(const Position &other) const {
    const auto dx = std::max(x, other.x) - std::min(x, other.x);
    const auto dy = std::max(y, other.y) - std::min(y, other.y);
    return dx + dy;
  }
};

std::ostream &operator<<(std::ostream &stream, const Position &p) {
  stream << p.x << ", " << p.y;
  return stream;
}

enum class ItemType { None, Radar, Trap, Ore };

ItemType readItemType() {
  switch (read<S32>()) {
  case -1:
    return ItemType::None;
  case 2:
    return ItemType::Radar;
  case 3:
    return ItemType::Trap;
  case 4:
    return ItemType::Ore;
  }
  throw std::runtime_error("Not a valid item type.");
}

std::string itemTypeToString(const ItemType type) {
  switch (type) {
  case ItemType::None:
    return "NONE";
  case ItemType::Radar:
    return "RADAR";
  case ItemType::Trap:
    return "TRAP";
  case ItemType::Ore:
    return "ORE";
  }
  throw std::runtime_error("Unhandled case.");
}

enum class ActionType { None, Wait, Move, Dig, Request };

std::string actionTypeToString(const ActionType type) {
  switch (type) {
  case ActionType::None:
    return "NONE";
  case ActionType::Wait:
    return "WAIT";
  case ActionType::Move:
    return "MOVE";
  case ActionType::Dig:
    return "DIG";
  case ActionType::Request:
    return "REQUEST";
  }
  throw std::runtime_error("Unhandled case.");
}

struct Action {
  ActionType type = ActionType::None;
  std::optional<Position> p;
  std::optional<ItemType> itemType;

  explicit Action(ActionType actionType) : type(actionType) {}

  [[nodiscard]] std::string toString() const {
    std::stringstream stream;
    stream << actionTypeToString(type);
    if (p) {
      stream << ' ' << p->x << ' ' << p->y;
    }
    if (itemType) {
      stream << ' ' << itemTypeToString(itemType.value());
    }
    return stream.str();
  }
};

struct Entity {
  U32 id = -1;
  EntityType type = EntityType::None;
  bool dead = false;
  Position p;
  ItemType item = ItemType::None;
  std::vector<Action> actions;
};

Entity readEntity() {
  Entity result;
  result.id = read<U32>();
  result.type = readEntityType();
  const auto x = read<S32>();
  if (x == -1) {
    result.dead = true;
  } else {
    result.p.x = static_cast<U32>(x);
  }
  const auto y = read<S32>();
  if (y == -1) {
    result.dead = true;
  } else {
    result.p.y = static_cast<U32>(y);
  }
  result.item = readItemType();
  return result;
}

class Game {
  Map map;

  U32 m = 0;
  U32 n = 0;

  std::vector<Entity> entities;

  std::vector<std::vector<F32>> estimatedOreAmount;

  U32 myScore = 0;
  U32 opponentScore = 0;

  U32 radarCooldown = 0;
  U32 trapCooldown = 0;

  void updateEstimates() {
    std::vector<Position> successfulDigging;
    std::vector<Position> unsuccessfulDigging;
    for (auto &entity : entities) {
      if (entity.type == EntityType::MyRobot) {
        if (!entity.dead) {
          if (!entity.actions.empty()) {
            if (entity.actions.back().type == ActionType::Dig) {
              const auto actionPosition = entity.actions.back().p.value();
              if (entity.item == ItemType::Ore) {
                successfulDigging.push_back(actionPosition);
              } else {
                unsuccessfulDigging.push_back(actionPosition);
              }
            }
          }
        }
      }
    }
    for (U32 i = 0; i < m; i++) {
      for (U32 j = 0; j < n; j++) {
        if (map.hasOreCount(i, j)) {
          // Use the ground truth, if available.
          estimatedOreAmount[i][j] = static_cast<float>(map.getOreCount(i, j));
        } else {
          const Position p(j, i);
          // Update our estimate if no ground truth is available.
          for (const auto position : successfulDigging) {
            if (position == p) {
              std::cerr << "Setting estimate @ (" << p << ") to 1/2." << '\n';
              estimatedOreAmount[i][j] = 0.5f;
            }
          }
          for (const auto position : unsuccessfulDigging) {
            if (position == p) {
              std::cerr << "Setting estimate @ (" << p << ") to 0." << '\n';
              estimatedOreAmount[i][j] = 0.0f;
            }
          }
        }
      }
    }
  }

public:
  Game(U32 width, U32 height) : map(width, height) {
    m = map.getHeight();
    n = map.getWidth();
    estimatedOreAmount.resize(m, std::vector<F32>(n));
    F64 total = 0.0;
    for (U32 i = 0; i < m; i++) {
      for (U32 j = 0; j < n; j++) {
        const auto ci = (m - 1.0) / 2.0;
        const auto cj = (n - 1.0) / 2.0;
        const auto sigma = std::min(m, n) / 2;
        const auto xSquared = std::pow(static_cast<F64>(j) - cj, 2.0);
        const auto ySquared = std::pow(static_cast<F64>(i) - ci, 2.0);
        const auto sumOfSquares = -(xSquared + ySquared);
        const auto sigmaSquared = std::pow(sigma, 2.0);
        const auto exponent = std::exp(sumOfSquares / (2.0 * sigmaSquared));
        const auto pi = 4.0 * std::atan(1.0);
        const auto gaussian = 1.0 / (2.0 * pi * sigmaSquared) * exponent;
        estimatedOreAmount[i][j] = gaussian;
        total += estimatedOreAmount[i][j];
      }
    }
    for (U32 i = 0; i < m; i++) {
      for (U32 j = 0; j < n; j++) {
        estimatedOreAmount[i][j] *= ((m * n) / total);
      }
    }
    std::ios_base::fmtflags formatFlags(std::cerr.flags());
    std::cerr << std::fixed;
    std::cerr << std::setprecision(2);
    for (U32 i = 0; i < m; i++) {
      for (U32 j = 0; j < n; j++) {
        if (j > 0) {
          std::cerr << ' ';
        }
        std::cerr << estimatedOreAmount[i][j];
      }
      std::cerr << '\n';
    }
    std::cerr.flags(formatFlags);

    for (U32 i = 0; i < m; i++) {
      estimatedOreAmount[i][0] = 0.0f;
    }
  }

  void updateMap() { map.readUpdate(); }

  void updateEntities() {
    const auto entityCount = read<U32>();
    radarCooldown = read<U32>();
    trapCooldown = read<U32>();
    for (U32 i = 0; i < entityCount; i++) {
      const auto entity = readEntity();
      auto updatedAnExistingEntity = false;
      for (auto &existingEntity : entities) {
        if (entity.id == existingEntity.id) {
          existingEntity.type = entity.type;
          existingEntity.dead = entity.dead;
          existingEntity.p = entity.p;
          existingEntity.item = entity.item;
          updatedAnExistingEntity = true;
          break;
        }
      }
      if (!updatedAnExistingEntity) {
        entities.push_back(entity);
      }
    }
    for (U32 i = 0; i < entityCount; i++) {
      if (entities[i].type == EntityType::TheirRobot) {
        if (entities[i].item != ItemType::None) {
          const auto itemType = itemTypeToString(entities[i].item);
          std::cerr << "Entity " << entities[i].id << ' ';
          std::cerr << "has " << itemType << '.' << std::endl;
        }
      }
    }
  }

  void updateState() {
    myScore = read<U32>();
    opponentScore = read<U32>();
    updateMap();
    updateEntities();
  }

  void considerGettingRadar(Entity &entity, Action &action) {
    if (entity.p.x == 0) {
      if (radarCooldown == 0) {
        action.type = ActionType::Request;
        action.itemType = ItemType::Radar;
      }
    }
  }

  void considerDigging(Entity &entity, Action &action) {
    std::optional<Position> best;
    for (U32 i = 0; i < m; i++) {
      for (U32 j = 0; j < n; j++) {
        if (estimatedOreAmount[i][j] > 0.0f) {
          Position q(j, i);
          if (!best) {
            best = q;
          }
          const auto currentDistance = entity.p.distanceTo(best.value());
          const auto alternativeDistance = entity.p.distanceTo(q);
          if (alternativeDistance < currentDistance) {
            best = q;
          }
        }
      }
    }
    if (best.has_value()) {
      action.type = ActionType::Dig;
      action.p = best;
    }
  }

  void moveEntities() {
    updateEstimates();
    for (auto &entity : entities) {
      if (entity.type == EntityType::MyRobot) {
        Action action(ActionType::Wait);
        if (entity.dead) {
        } else if (entity.item == ItemType::Ore) {
          action.type = ActionType::Move;
          action.p = Position(0, entity.p.y);
        } else if (entity.item == ItemType::Radar) {
          action.type = ActionType::Dig;
          std::optional<Position> whereToDig;
          if (!entity.actions.empty()) {
            if (entity.actions.back().type == ActionType::Dig) {
              whereToDig = entity.actions.back().p;
            }
          }
          if (whereToDig) {
            action.p = whereToDig.value();
          } else {
            action.p = Position(std::rand() % n, std::rand() % m);
          }
        } else {
          considerGettingRadar(entity, action);
          if (action.type == ActionType::Wait) {
            considerDigging(entity, action);
          }
        }
        std::cout << action.toString() << std::endl;
        entity.actions.push_back(action);
      }
    }
  }
};

int main() {
  const auto width = read<U32>();
  const auto height = read<U32>();
  Game game(width, height);
  auto running = true;
  while (running) {
    game.updateState();
    game.moveEntities();
  }
}
