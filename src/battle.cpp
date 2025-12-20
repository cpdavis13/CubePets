#include "battle.h"
#include "sprite.h"
#include "display.h"
#include "esp_system.h"

Battle::Battle(TFT_eSPI* tft, uint16_t width, uint16_t height)
    : characters(), comm(this), map(), display(nullptr), battle_tft(tft), myMac(0)
{
    display = new Display(&map, tft, width, height);
}

Battle::~Battle() {
    delete display;
}

void Battle::init() {
    comm.begin(115200);
    myMac = comm.getMyMac();
    display->setMac(myMac);
    map.addCube(myMac, 0, -1);

    uint32_t seed = esp_random();
    srand(seed);  // Seed your PRNG with high-quality entropy

    CharacterPtr newCharacter(new Character(myMac, 0, battle_tft, &map, this));
    addCharacter(newCharacter);
    

    display->setCharacters(characters);
}


void Battle::update() {
    comm.update();

    if (comm.getRole() == ROLE_UNASSIGNED) {
        updateCharacters();
    }

    if (comm.getRole() == ROLE_HOST) {
        updateCharacters();
        sendCommands();
    }

    display->setCharacters(characters);
    display->draw(characters);
}

void Battle::addCharacter(CharacterPtr character) {
    characters.push_back(character);
}

void Battle::createCharacter(uint32_t senderMac, uint8_t id) {
    for (const auto& character : characters) {
        if (character->getMac() == senderMac) {
            return;
        }
    }
    CharacterPtr newCharacter(new Character(senderMac, id, battle_tft, &map, this));
    addCharacter(newCharacter);
    newCharacter->setPosition(160, 60);
}

void Battle::updateCharacters() {
    for (auto& c : characters) {
        c->update();
    }
}

void Battle::sendCommands() {
    std::vector<uint8_t> payload;
    for (const auto& character : characters) {
        uint32_t mac = character->getMac();
        payload.push_back((mac >> 24) & 0xFF);
        payload.push_back((mac >> 16) & 0xFF);
        payload.push_back((mac >> 8) & 0xFF);
        payload.push_back(mac & 0xFF);

        payload.push_back(character->getId());
        payload.push_back(character->getX());
        payload.push_back(character->getY());
        payload.push_back(character->getSprite()->getFrame());
    }

    comm.sendPacketToNeighbors(PACKET_COMMAND, payload.data(), payload.size());
}

void Battle::processCommands(const std::vector<uint8_t>& payload) {
    size_t charDataSize = 8; // bytes per character
    for (size_t i = 0; i + charDataSize <= payload.size(); i += charDataSize) {
        uint32_t mac = (payload[i] << 24) |
                (payload[i + 1] << 16) |
                (payload[i + 2] << 8) |
                payload[i + 3];

        uint8_t id = payload[i + 4];
        uint8_t x = payload[i + 5];
        uint8_t y = payload[i + 6];
        uint8_t frame = payload[i + 7];

        Character* c = findCharacterByMac(mac);
        if (c) {
            c->clientUpdate(x, y, frame);
        } else {
            CharacterPtr newCharacter(new Character(mac, id, battle_tft, &map, this));
            addCharacter(newCharacter);
            newCharacter->clientUpdate(x, y, frame);
            display->setCharacters(characters);
        }
    }
}

Character* Battle::findCharacterByMac(uint32_t mac) {
    for (const auto& character : characters) {
        if (character->getMac() == mac) {
            return character.get();
        }
    }
    return nullptr;
}

void Battle::addCube(uint32_t newMac, int sideFromThis) {
  map.addCube(newMac, myMac, sideFromThis);
}

void Battle::removeCube(uint32_t mac) {
    map.removeCube(mac);
}

void Battle::addCubeWithPath(uint32_t mac, const std::vector<uint8_t>& path) {
    map.addCubeWithPath(mac, path);

    if (mac == myMac) {
        const Cube* c = map.getCubeInfo(myMac);
        if (c) {
            display->setViewOrigin(c->x, c->y);
        }
    }
}


std::vector<uint8_t> Battle::getPathFromHost() const {
    return map.getPathFromHost(myMac);
}

bool Battle::isCharacterInCubeBounds(uint32_t cubeMac, int16_t charX, int16_t charY) const {
    return map.isCharacterInCubeBounds(cubeMac, charX, charY);
}

uint32_t Battle::getMyMac() {
    return myMac;
}

Character* Battle::findNearestEnemy(Character* seeker) {
    Character* nearest = nullptr;
    float nearestDist = 999999.0f;

    for (const auto& characterPtr : characters) {
        Character* candidate = characterPtr.get();
        if (candidate == seeker) continue;        // Skip self
        if (!candidate->isAlive()) continue;         // Skip dead

        float dx = candidate->getX() - seeker->getX();
        float dy = candidate->getY() - seeker->getY();
        float dist = sqrtf(dx*dx + dy*dy);

        if (dist < nearestDist) {
            nearestDist = dist;
            nearest = candidate;
        }
    }
    return nearest;
}

std::vector<Character*> Battle::findEnemiesInRange(Character* seeker, float range) {
    std::vector<Character*> enemies;
    for (const auto& characterPtr : characters) {
        Character* candidate = characterPtr.get();
        if (candidate == seeker) continue;
        if (!candidate->isAlive()) continue;

        float dx = candidate->getX() - seeker->getX();
        float dy = candidate->getY() - seeker->getY();
        float dist = sqrtf(dx*dx + dy*dy);

        if (dist <= range) {
            enemies.push_back(candidate);
        }
    }
    return enemies;
}
