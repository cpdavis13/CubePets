#pragma once
#include <vector>
#include <memory>
#include "TFT_eSPI.h"
#include "Comms.h"
#include "Map.h"
#include"character.h"

using CharacterPtr = std::shared_ptr<Character>;
using CharacterList = std::vector<CharacterPtr>;

class Display;  // forward declaration

class Battle {
public:
    Battle(TFT_eSPI* tft, uint16_t width, uint16_t height);
    ~Battle();

    void init();
    void update();

    void createCharacter(uint32_t senderMac, uint8_t id);
    void addCharacter(CharacterPtr character);
    void updateCharacters();
    void sendCommands();
    void processCommands(const std::vector<uint8_t>& payload);
    Character* findCharacterByMac(uint32_t mac);

    void addCube(uint32_t mac, int sideFromThis);
    void removeCube(uint32_t mac);
    void addCubeWithPath(uint32_t mac, const std::vector<uint8_t>& path);
    std::vector<uint8_t> getPathFromHost() const;

    bool isCharacterInCubeBounds(uint32_t cubeMac, int16_t charX, int16_t charY) const;
    Character* findNearestEnemy(Character* seeker);
    std::vector<Character*> findEnemiesInRange(Character* seeker, float range);
    uint32_t getMyMac();

private:
    CharacterList characters;
    Comms comm;
    Map map;
    Display* display;
    TFT_eSPI* battle_tft;
    uint32_t myMac;
};
