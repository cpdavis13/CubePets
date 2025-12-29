#include "character.h"
#include "battle.h"
#include <cstdlib>      // rand(), srand()
#include <cmath>

Character::Character(uint32_t mac, uint8_t id, TFT_eSPI* tft, Map* map, Battle* battle)
  : _mac(mac), _id(id), _map(map), _battle(battle), _x(0), _y(0), _zOrder(1)
{
  _sprite = new Sprite(tft);
  _sprite->load(id);
  _sprite->play("idle");

  _health = 100;
  _maxHealth = 100;
  _attackPower = 10;
  _defense = 2;
  _alive = true;
  _state = WANDER;
  _attackCooldown = 1000; // 1 second
  _lastAttackTime = 0;
  _fleeThreshold = 20;
}

Character::~Character() {
  delete _sprite;
}

void Character::update() {
    if (!_alive) {
        if (_currentAnimation != "death") {
            _sprite->play("death", false);
            _currentAnimation = "death";
        }
        _sprite->update();
        return;
    }

    updateAI();
    _sprite->update();
}


void Character::updateAI() {
  unsigned long now = millis();

  if (_health <= 0) {
    _alive = false;
    _state = DEAD;
    return;
  }

  Character* target = findNearestEnemy();
  float dist = (target) ? distanceTo(target) : 9999;

  if (_health < _fleeThreshold) {
    _state = FLEE;
  } else if (target && dist <= _attackRange) {
    _state = ATTACK;
  } else if (target && dist <= _detectionRange) {
    _state = CHASE;
  } else {
    _state = WANDER;
  }

  switch (_state) {
    case WANDER:
      wanderRandomly();
      break;
    case CHASE:
      moveToward(target);
      break;
    case ATTACK:
      if (canAttack()) {
        performAttack(target);
      }
      break;
    case FLEE: {
      // Get all nearby enemies for fleeing
      std::vector<Character*> enemiesNearby = _battle->findEnemiesInRange(this, _detectionRange);
      moveAwayFromGroup(enemiesNearby);
      break;
    }
    default:
      break;
  }
}


Character* Character::findNearestEnemy() {
  return _battle->findNearestEnemy(this); // You need to implement this in your Map class
}

float Character::distanceTo(Character* other) {
  float dx = other->getX() - _x;
  float dy = other->getY() - _y;
  return sqrtf(dx * dx + dy * dy);
}

bool Character::canAttack() {
  return millis() - _lastAttackTime >= _attackCooldown;
}

void Character::performAttack(Character* target) {
  int damage = _attackPower - target->_defense;
  if (damage < 1) damage = 1;
  target->takeDamage(damage);
  _lastAttackTime = millis();
}

void Character::takeDamage(int dmg) {
  _health -= dmg;
  if (_health <= 0) {
    _health = 0;
    _alive = false;
  }
}

void Character::wanderRandomly() {
  unsigned long now = millis();

  if (now - _lastDirChange > _directionInterval) {
    float angle = ((rand() / (float)RAND_MAX) * 2.0f * PI);
    _ndx = cosf(angle);
    _ndy = sinf(angle);
    _movement = _speed * 2.0f;
    _lastDirChange = now;
  }

  moveByDirection();
}

void Character::moveToward(Character* target) {
  float dx = target->getX() - _x;
  float dy = target->getY() - _y;
  float len = sqrtf(dx * dx + dy * dy);
  _ndx = dx / len;
  _ndy = dy / len;
  _movement = _speed * 2.0f;

  moveByDirection();
}

void Character::moveAwayFrom(Character* target) {
  float dx = _x - target->getX();
  float dy = _y - target->getY();
  float len = sqrtf(dx * dx + dy * dy);
  _ndx = dx / len;
  _ndy = dy / len;
  _movement = _speed * 2.5f;

  moveByDirection();
}

void Character::moveAwayFromGroup(const std::vector<Character*>& targets) {
    if (targets.empty()) {
        // No targets, just wander or do nothing
        return;
    }

    // Compute average position of all targets
    float avgX = 0.0f;
    float avgY = 0.0f;
    int count = 0;

    for (auto target : targets) {
        if (target && target->isAlive()) {
            avgX += target->getX();
            avgY += target->getY();
            count++;
        }
    }

    if (count == 0) {
        return; // no alive targets
    }

    avgX /= count;
    avgY /= count;

    // Calculate vector away from average position
    float dx = _x - avgX;
    float dy = _y - avgY;
    float len = sqrtf(dx * dx + dy * dy);

    if (len == 0) {
        // Exactly on average position, pick random direction
        float angle = ((rand() / (float)RAND_MAX) * 2.0f * M_PI);
        _ndx = cosf(angle);
        _ndy = sinf(angle);
    } else {
        _ndx = dx / len;
        _ndy = dy / len;
    }

    _movement = _speed * 2.5f;

    moveByDirection();
}

void Character::moveByDirection() {
  unsigned long now = millis();

  if (now - _lastMoveTime > _moveInterval) {
    int16_t newX = _x + (int16_t)(_ndx * _movement);
    int16_t newY = _y + (int16_t)(_ndy * _movement);
    int16_t width = _sprite->getFrameWidth();
    int16_t height = _sprite->getFrameHeight();

    if (_map->isCharacterInWorld(newX, newY) &&
        _map->isCharacterInWorld(newX + width - 1, newY) &&
        _map->isCharacterInWorld(newX, newY + height - 1) &&
        _map->isCharacterInWorld(newX + width - 1, newY + height - 1)) {
        _x = newX;
        _y = newY;
    }

    _lastMoveTime = now;
  }
}

bool Character::isAlive() {
  return _alive;
}

void Character::setPosition(int16_t x, int16_t y) {
  _x = x;
  _y = y;
}

void Character::clientUpdate(uint8_t x, uint8_t y, uint8_t frame) {
  setPosition(x, y);
  _sprite->setFrame(frame);
}

uint8_t Character::getId() const {return _id;}
int16_t Character::getX() const { return _x; }
int16_t Character::getY() const { return _y; }
uint8_t Character::getZOrder() const { return _zOrder; }
Sprite* Character::getSprite() const { return _sprite; }

void Character::setMaxHealth(int maxHealth) {_maxHealth = maxHealth; _health = maxHealth;}
void Character::setSpeed(float speed) {_speed = speed;}
void Character::setPower(int power) {_attackPower = power;}
void Character::setDefense(int defense) {_defense = defense;}

uint32_t Character::getMac() {return _mac;}