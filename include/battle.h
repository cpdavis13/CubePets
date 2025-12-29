// Battle.h
//
// The central authority for character lifecycle, map integration, networking, and rendering.

#ifndef BATTLE_H
#define BATTLE_H

#pragma once
#include <vector>
#include <memory>
#include "TFT_eSPI.h"
#include "Comms.h"
#include "Map.h"
#include"character.h"

using CharacterPtr = std::shared_ptr<Character>; //shared ownership across battle instances
using CharacterList = std::vector<CharacterPtr>;

class Display;  // forward declaration

class Battle {
public:
    //Creates a battle instance without taking ownership of TFT_eSPI object
    Battle(TFT_eSPI* tft, uint16_t width, uint16_t height);
    ~Battle();

    // Initializes networking, local identity, display, RNG and local character
    void init();

    //Advances game state, handles networking and renders frame
    void update();

    void createCharacter(uint32_t senderMac, uint8_t id);
    void addCharacter(CharacterPtr character);
    void updateCharacters();

    //Send character state to neighboring cubes
    void sendCommands();

    //Apply receives character state
    //Payload must match sendCommands()
    void processCommands(const std::vector<uint8_t>& payload);

    //Returns nullptr if not found
    Character* findCharacterByMac(uint32_t mac);

    //Registers a new cube relative to this device
    void addCube(uint32_t mac, int sideFromThis);

    //Removes cube from network topology
    void removeCube(uint32_t mac);

    //Adds a cube with a known path to host
    void addCubeWithPath(uint32_t mac, const std::vector<uint8_t>& path);

    //Returns the path from host to this device
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

#endif //BATTLE_H