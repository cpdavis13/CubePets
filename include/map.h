#pragma once
#include <map>
#include <vector>
#include <cstdint>


struct Cube {
    int16_t x;
    int16_t y;
    bool isHost;
    std::vector<uint8_t> pathFromHost;
};

class Map {
public:
    Map();

    const Cube* getCubeInfo(uint32_t mac) const;
    void addCube(uint32_t newMac, uint32_t referenceMac, int sideFromThis);
    void addCubeWithPath(uint32_t mac, const std::vector<uint8_t>& path);
    void removeCube(uint32_t mac);

    std::vector<uint8_t> getPathFromHost(uint32_t mac) const;
    bool isCharacterInCubeBounds(uint32_t cubeMac, int16_t charX, int16_t charY) const;
    bool isCharacterInWorld(int16_t charX, int16_t charY) const;

    void setMyMac(uint32_t mac);

private:
    std::map<uint32_t, Cube> cubes;
    uint32_t myMac = 0;
};
