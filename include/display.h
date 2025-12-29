#pragma once
#include <vector>
#include <memory>
#include "Character.h"  
#include "TFT_eSPI.h"

using CharacterPtr = std::shared_ptr<Character>;
using CharacterList = std::vector<CharacterPtr>;

class Map;  //Forward declaration

class Display {
public:
    Display(Map* map, TFT_eSPI* tft, uint16_t width, uint16_t height);
    ~Display();

    void setViewOrigin(int16_t originX, int16_t originY);
    void setCharacters(const CharacterList& characters);
    void draw(const CharacterList& characters);
    void setMac(uint32_t mac);

private:
    Map* _map;
    TFT_eSPI* _tft;
    uint16_t _width, _height;
    TFT_eSprite _buffer;
    int16_t _originX = 0;
    int16_t _originY = 0;

    CharacterList _sortedCharacters;
    bool _needsResort = true;

    int32_t myMac = 0;
};
