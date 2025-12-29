#include "map.h"
#include <Arduino.h> // for Serial.printf

static const int16_t sideOffsetsX[] = {128, 0, -128, 0};
static const int16_t sideOffsetsY[] = {0, -128, 0, 128};

Map::Map() {}

void Map::addCube(uint32_t newMac, uint32_t referenceMac, int sideFromThis) {
    if (cubes.count(newMac)) return;

    int16_t x = 0;
    int16_t y = 0;

    if (sideFromThis != -1 && cubes.count(referenceMac)) {
        auto it = cubes.find(referenceMac);
        x = it->second.x;
        y = it->second.y;

        switch (sideFromThis) {
            case 0: x += 128; break;
            case 1: y -= 128; break;
            case 2: x -= 128; break;
            case 3: y += 128; break;
        }
    }
    // else x,y = 0 (root cube)   

    cubes[newMac] = {x, y, (newMac == myMac), {}};
    Serial.printf("[MAP] Added cube %u at (%d, %d)\n", newMac, x, y);
}



void Map::addCubeWithPath(uint32_t mac, const std::vector<uint8_t>& path) {
    if (mac == 0) return;

    if (path.empty()) {
        // Host cube at 0,0
        cubes[mac] = {0, 0, true, {}};
        Serial.printf("[MAP] Added host cube %u at (0,0)\n", mac);
        return;
    }

    int16_t x = 0;
    int16_t y = 0;
    for (uint8_t side : path) {
        if (side < 1) {                   //UPDATE TO NUMBER OF SIDES
            x += sideOffsetsX[side];
            y += sideOffsetsY[side];
        } else {
            Serial.printf("[MAP][WARN] Invalid side %u in path\n", side);
        }
    }

    cubes[mac] = {x, y, false, path};
    Serial.printf("[MAP] Added cube %u at (%d, %d) with path length %zu\n", mac, x, y, path.size());
}

void Map::removeCube(uint32_t mac) {
    auto it = cubes.find(mac);
    if (it != cubes.end()) {
        Serial.printf("[MAP] Removed cube %u from (%d, %d)\n", mac, it->second.x, it->second.y);
        cubes.erase(it);
    }
}

std::vector<uint8_t> Map::getPathFromHost(uint32_t mac) const {
    auto it = cubes.find(mac);
    if (it != cubes.end()) {
        return it->second.pathFromHost;
    }
    return {};
}

bool Map::isCharacterInWorld(int16_t charX, int16_t charY) const {
    for (const auto& pair : cubes) {
        const Cube& cube = pair.second;
        int16_t x = cube.x;
        int16_t y = cube.y;

        if (charX >= x && charX < x + 128 &&
            charY >= y && charY < y + 128) {
            return true;
        }
    }
    return false;
}

bool Map::isCharacterInCubeBounds(uint32_t cubeMac, int16_t charX, int16_t charY) const {
    auto it = cubes.find(cubeMac);
    if (it == cubes.end()) return false;

    const Cube& cube = it->second;
    const int16_t cubeWidth = 128;
    const int16_t cubeHeight = 128;

    return (charX >= cube.x && charX < cube.x + cubeWidth) &&
           (charY >= cube.y && charY < cube.y + cubeHeight);
}

//Returns pointer to Cube info if found, otherwise nullptr
const Cube* Map::getCubeInfo(uint32_t mac) const {
    auto it = cubes.find(mac);
    if (it != cubes.end()) {
        return &(it->second);
    }
    return nullptr;
}

void Map::setMyMac(uint32_t mac) {myMac = mac;}