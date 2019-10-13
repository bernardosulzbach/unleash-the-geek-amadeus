#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ios>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;

using S32 = int32_t;

using F32 = float;
using F64 = double;

using BooleanMatrix = std::vector<std::vector<bool>>;

constexpr U32 RadarRadius = 4;

template <typename T> T read() {
  T t;
  std::cin >> t;
  return t;
}

void printMatrix(const std::vector<std::vector<F32>> &matrix) {
  std::ios_base::fmtflags formatFlags(std::cerr.flags());
  std::cerr << std::fixed;
  std::cerr << std::setprecision(2);
  const auto m = static_cast<U32>(matrix.size());
  for (U32 i = 0; i < m; i++) {
    const auto n = static_cast<U32>(matrix.front().size());
    for (U32 j = 0; j < n; j++) {
      if (j > 0) {
        std::cerr << ' ';
      }
      std::cerr << matrix[i][j];
    }
    std::cerr << '\n';
  }
  std::cerr.flags(formatFlags);
}

struct Cell {
  std::optional<U8> ore;
  bool hasHole = false;
  U16 holeAge = 0;
};

class Map {
  std::vector<std::vector<Cell>> cells;

public:
  Map(U32 width, U32 height) : cells(height, std::vector<Cell>(width)) {}

  [[nodiscard]] U32 getHeight() const { return cells.size(); }

  [[nodiscard]] U32 getWidth() const { return cells.front().size(); }

  [[nodiscard]] U32 getHoleAge(U32 i, U32 j) const {
    return cells[i][j].holeAge;
  }

  [[nodiscard]] bool hasHole(U32 i, U32 j) const { return cells[i][j].hasHole; }

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
        if (cells[i][j].hasHole) {
          cells[i][j].holeAge++;
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

  [[nodiscard]] U32 turnsToDigAt(const Position &other) const {
    const auto distance = distanceTo(other);
    if (distance <= 1) {
      return 1;
    }
    return 1 + (distance - 1 + 3) / 4;
  }

  [[nodiscard]] U32 turnsToDigAtAndReturn(const Position &other) const {
    return turnsToDigAt(other) + (other.x + 3) / 4;
  }
};

std::ostream &operator<<(std::ostream &stream, const Position &p) {
  stream << "(" << p.x << ", " << p.y << ")";
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
  Position previousP;
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

  std::vector<std::vector<F32>> trapProbability;
  std::vector<std::vector<F32>> estimatedOreAmount;

  BooleanMatrix hasDigger;

  std::unordered_set<U32> suspects;

  U32 myScore = 0;
  U32 opponentScore = 0;

  U32 radarCooldown = 0;
  U32 trapCooldown = 0;

  [[nodiscard]] BooleanMatrix getRadarCoverage() const {
    std::vector<Position> radars;
    for (const auto &entity : entities) {
      if (entity.type == EntityType::Radar) {
        radars.push_back(entity.p);
      }
    }
    std::vector<std::vector<bool>> coverage(m, std::vector<bool>(n));
    for (U32 i = 0; i < m; i++) {
      for (U32 j = 0; j < n; j++) {
        const auto p = Position(j, i);
        for (const auto radarPosition : radars) {
          if (p.distanceTo(radarPosition) <= RadarRadius) {
            coverage[i][j] = true;
          }
        }
      }
    }
    return coverage;
  }

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
              std::cerr << "Setting estimate @ " << p << " to 1/2." << '\n';
              const auto afterMining = estimatedOreAmount[i][j] - 1.0f;
              estimatedOreAmount[i][j] = std::max(0.0f, afterMining);
            }
          }
          for (const auto position : unsuccessfulDigging) {
            if (position == p) {
              std::cerr << "Setting estimate @ " << p << " to 0." << '\n';
              estimatedOreAmount[i][j] = 0.0f;
              continue;
            }
          }
          if (map.hasHole(i, j)) {
            if (map.getHoleAge(i, j) == 0) {
              if (estimatedOreAmount[i][j] >= 1.0f) {
                estimatedOreAmount[i][j] -= 1.0f;
              } else {
                estimatedOreAmount[i][j] *= 0.5f;
              }
              trapProbability[i][j] = std::max(trapProbability[i][j], 0.5f);
            }
          }
        }
      }
    }
  }

  [[nodiscard]] std::optional<Position> findDiggingPosition(Entity &entity) {
    std::optional<Position> best;
    for (U32 i = 0; i < m; i++) {
      for (U32 j = 1; j < n; j++) {
        if (estimatedOreAmount[i][j] == 0.0f) {
          continue;
        }
        if (hasDigger[i][j]) {
          continue;
        }
        Position other(j, i);
        if (!best) {
          best = other;
          continue;
        }
        const auto bestTrapProbability = trapProbability[best->y][best->x];
        const auto otherTrapProbability = trapProbability[other.y][other.x];
        if (otherTrapProbability < bestTrapProbability) {
          best = other;
          continue;
        }
        if (otherTrapProbability > bestTrapProbability) {
          continue;
        }
        const auto bestEstimate = estimatedOreAmount[best->y][best->x];
        const auto otherEstimate = estimatedOreAmount[other.y][other.x];
        const auto changeIfAsClose = [&entity, &best, other]() {
          const auto bestTurns = entity.p.turnsToDigAtAndReturn(best.value());
          const auto otherTurns = entity.p.turnsToDigAtAndReturn(other);
          if (otherTurns <= bestTurns) {
            best = other;
          }
        };
        if (bestEstimate < 1.0f) {
          if (otherEstimate >= 1.0f) {
            best = other;
          } else if (otherEstimate > bestEstimate) {
            changeIfAsClose();
          }
        } else {
          if (otherEstimate >= 1.0f) {
            changeIfAsClose();
          }
        }
      }
    }
    if (best) {
      std::cerr << "Found digging @ " << best.value() << "." << '\n';
      const auto trapP = trapProbability[best->y][best->x];
      std::cerr << "  Trap: " << trapP << "." << '\n';
      const auto oreE = estimatedOreAmount[best->y][best->x];
      std::cerr << "  Ore:  " << oreE << "." << '\n';
    }
    return best;
  }

  [[nodiscard]] F32 evaluateRadarScore(U32 i, U32 j,
                                       const BooleanMatrix &coverage) const {
    F32 score = 0.0f;
    const auto sI = static_cast<S32>(i);
    const auto sJ = static_cast<S32>(j);
    const auto signedRadius = static_cast<S32>(RadarRadius);
    for (auto pI = sI - signedRadius; pI <= sI + signedRadius; pI++) {
      for (auto pJ = sJ - signedRadius; pJ <= sJ + signedRadius; pJ++) {
        const auto signedM = static_cast<S32>(m);
        const auto signedN = static_cast<S32>(n);
        if (pI < 0 || pI >= signedM || pJ < 0 || pJ >= signedN) {
          continue;
        }
        const auto dI = std::max(sI, pI) - std::min(sI, pI);
        const auto dJ = std::max(sJ, pJ) - std::min(sJ, pJ);
        if (dI + dJ > signedRadius) {
          continue;
        }
        if (coverage[pI][pJ]) {
          continue;
        }
        score += 1.0f;
      }
    }
    return score;
  }

  [[nodiscard]] std::optional<Position> findRadarPosition() {
    std::optional<Position> best;
    auto bestScore = 0.0f;
    auto bestTrapProbability = 0.0f;
    const auto coverage = getRadarCoverage();
    for (U32 j = 1; j < n; j++) {
      for (U32 i = 0; i < m; i++) {
        if (trapProbability[i][j] == 1.0f) {
          continue;
        }
        const auto otherScore = evaluateRadarScore(i, j, coverage);
        const auto otherTrapProbability = trapProbability[i][j];
        if (!best) {
          best = Position(j, i);
          bestScore = otherScore;
          bestTrapProbability = otherTrapProbability;
          continue;
        }
        if (otherTrapProbability < bestTrapProbability) {
          best = Position(j, i);
          bestScore = otherScore;
          bestTrapProbability = otherTrapProbability;
        } else if (otherTrapProbability == bestTrapProbability) {
          if (otherScore > bestScore) {
            best = Position(j, i);
            bestScore = otherScore;
            bestTrapProbability = otherTrapProbability;
          }
        }
      }
    }
    if (best) {
      std::cerr << "Found radar position @" << ' ';
      std::cerr << best.value() << ' ';
      std::cerr << "with a score of " << bestScore << "." << '\n';
    }
    return best;
  }

  void increaseTrapSuspicion(const S32 x, const S32 y) {
    if (x < 0 || x >= static_cast<S32>(n)) {
      return;
    }
    if (y < 0 || y >= static_cast<S32>(m)) {
      return;
    }
    trapProbability[y][x] = std::min(1.0f, trapProbability[y][x] + 0.25f);
  }

  void increaseTrapSuspicion(const Position &p) {
    const auto sx = static_cast<S32>(p.x);
    const auto sy = static_cast<S32>(p.y);
    increaseTrapSuspicion(sx - 1, sy);
    increaseTrapSuspicion(sx, sy - 1);
    increaseTrapSuspicion(sx, sy);
    increaseTrapSuspicion(sx, sy + 1);
    increaseTrapSuspicion(sx + 1, sy);
  }

public:
  Game(U32 width, U32 height) : map(width, height) {
    m = map.getHeight();
    n = map.getWidth();
    estimatedOreAmount.resize(m, std::vector<F32>(n));
    trapProbability.resize(m, std::vector<F32>(n));
    hasDigger.resize(m, std::vector<bool>(n));
    for (U32 i = 0; i < m; i++) {
      for (U32 j = 0; j < n; j++) {
        estimatedOreAmount[i][j] = 0.95f * j / n;
      }
    }
    for (U32 i = 0; i < m; i++) {
      estimatedOreAmount[i][0] = 0.0f;
    }
    printMatrix(estimatedOreAmount);
  }

  void updateMap() {
    map.readUpdate();
    for (U32 i = 0; i < m; i++) {
      for (U32 j = 0; j < n; j++) {
        hasDigger[i][j] = false;
      }
    }
  }

  void updateEntities() {
    const auto entityCount = read<U32>();
    radarCooldown = read<U32>();
    trapCooldown = read<U32>();
    for (U32 i = 0; i < entityCount; i++) {
      const auto entity = readEntity();
      auto updatedAnExistingEntity = false;
      for (auto &existingEntity : entities) {
        const auto id = existingEntity.id;
        if (entity.id == id) {
          existingEntity.type = entity.type;
          if (!existingEntity.dead && entity.dead) {
            if (existingEntity.type == EntityType::MyRobot) {
              // One of ours just died.
              printMatrix(trapProbability);
            }
          }
          existingEntity.dead = entity.dead;
          existingEntity.previousP = existingEntity.p;
          existingEntity.p = entity.p;
          existingEntity.item = entity.item;
          if (existingEntity.type == EntityType::TheirRobot) {
            if (existingEntity.p == existingEntity.previousP) {
              // Stood still at the elevator.
              if (existingEntity.p.x == 0) {
                suspects.insert(id);
                std::cerr << "Added " << id << " as a suspect." << '\n';
              }
              // Stood still somewhere else.
              if (existingEntity.p.x > 0) {
                if (suspects.count(id) == 1) {
                  suspects.erase(id);
                  std::cerr << "Removed " << id << " as a suspect." << '\n';
                  increaseTrapSuspicion(existingEntity.p);
                }
              }
            }
          }
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

  void considerGettingTrap(Entity &entity, Action &action) {
    if (entity.p.x == 0) {
      if (trapCooldown == 0) {
        action.type = ActionType::Request;
        action.itemType = ItemType::Trap;
      }
    }
  }

  void considerDigging(Entity &entity, Action &action) {
    const auto best = findDiggingPosition(entity);
    if (best.has_value()) {
      action.type = ActionType::Dig;
      action.p = best;
      hasDigger[best->y][best->x] = true;
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
          const auto whereToDig = findRadarPosition();
          if (whereToDig) {
            action.type = ActionType::Dig;
            action.p = whereToDig.value();
          }
        } else if (entity.item == ItemType::Trap) {
          std::optional<Position> whereToDig;
          if (!entity.actions.empty()) {
            if (entity.actions.back().type == ActionType::Dig) {
              whereToDig = entity.actions.back().p;
            }
          }
          if (!whereToDig) {
            whereToDig = findDiggingPosition(entity);
          }
          if (whereToDig) {
            action.type = ActionType::Dig;
            action.p = whereToDig.value();
            // Assume we succeed at placing a trap there.
            trapProbability[action.p->y][action.p->x] = 1.0f;
          }
        } else {
          considerGettingRadar(entity, action);
          if (action.type == ActionType::Wait) {
            considerGettingTrap(entity, action);
          }
          if (action.type == ActionType::Wait) {
            considerDigging(entity, action);
          }
        }
        if (action.type == ActionType::Request) {
          if (action.itemType == ItemType::Radar) {
            radarCooldown = 5;
          } else if (action.itemType == ItemType::Trap) {
            trapCooldown = 5;
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
