#ifndef CHARACTER_H
#define CHARACTER_H

#include "sprite.h"
#include "map.h"

class Battle;

class Character {
public:
  Character(uint32_t mac, uint8_t id, TFT_eSPI* tft, Map* map, Battle* battle);
  ~Character();

  void update();
  void updateAI();
  void setPosition(int16_t x, int16_t y);
  void clientUpdate(uint8_t x, uint8_t y, uint8_t frame);

  void moveByDirection();
  void moveToward(Character* target);
  void moveAwayFrom(Character* target);
  void moveAwayFromGroup(const std::vector<Character*>& targets);
  void wanderRandomly();

  Character* findNearestEnemy();
  std::vector<Character*> findEnemiesInRange(float range);

  float distanceTo(Character* other);
  bool canAttack();
  void performAttack(Character* target);
  void takeDamage(int dmg);

  bool isAlive();

  uint8_t getIdx() const;
  uint8_t getId() const;
  int16_t getX() const;
  int16_t getY() const;
  uint8_t getZOrder() const;
  Sprite* getSprite() const;

  void setMaxHealth(int health);
  void setSpeed(float speed);
  void setPower(int power);
  void setDefense(int defense);

  uint32_t getMac();
  
private:
  uint32_t _mac;
  Sprite* _sprite;
  Map* _map;
  Battle* _battle = nullptr;
  int16_t _x, _y;
  uint8_t _zOrder;
  uint8_t _idx = 0;
  uint8_t _id = 0;

  float _ndx = 0.0f;
  float _ndy = 0.0f;
  float _movement = 0.0f;
  float _speed = 1.0f;

  unsigned long _lastDirChange = 0;
  unsigned long _lastMoveTime = 0;
  const uint16_t _moveInterval = 50;
  const uint16_t _directionInterval = 2000;

  int _health = 100;
  int _maxHealth = 100;
  int _attackPower = 10;
  int _defense = 2;
  unsigned long _lastAttackTime = 0;
  unsigned long _attackCooldown = 1000; // ms cooldown between attacks
  int _fleeThreshold = 20;               // health threshold to flee
  float _attackRange = 20.0f;            // range to attack
  float _detectionRange = 300.0f;         // range to detect enemies

  bool _alive = true;
  Character* _target = nullptr;
  String _currentAnimation = "null";

  enum State { IDLE, WANDER, CHASE, ATTACK, FLEE, DEAD } _state = WANDER;
};

#endif // CHARACTER_H
