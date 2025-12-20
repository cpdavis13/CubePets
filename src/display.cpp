#include "display.h"
#include "battle.h"  // include full definition here
#include <algorithm>

Display::Display(Map* map, TFT_eSPI* tft, uint16_t width, uint16_t height)
    : _map(map), _tft(tft), _width(width), _height(height), _buffer(tft)
{
    _buffer.createSprite(_width, _height);
}

Display::~Display() {
    _buffer.deleteSprite();
}

void Display::setViewOrigin(int16_t originX, int16_t originY) {
    _originX = originX;
    _originY = originY;
}

void Display::setCharacters(const CharacterList& characters) {
    _sortedCharacters = characters;
    _needsResort = true;
}

void Display::draw(const CharacterList& characters) {
    _buffer.fillSprite(TFT_BLACK);

    struct VisibleChar {
        CharacterPtr character;
        int16_t screenX;
        int16_t screenY;
    };
    std::vector<VisibleChar> visible;

    for (const auto& c : characters) {
        int16_t worldX = c->getX();
        int16_t worldY = c->getY();

        int16_t localX = worldX - _originX;
        int16_t localY = worldY - _originY;

        if (localX + c->getSprite()->getFrameWidth() > 0 && localX < _width &&
            localY + c->getSprite()->getFrameHeight() > 0 && localY < _height) {
            visible.push_back({ c, localX, localY });
        }
    }

    if (_needsResort) {
        std::sort(_sortedCharacters.begin(), _sortedCharacters.end(),
            [](const CharacterPtr& a, const CharacterPtr& b) {
                return a->getZOrder() < b->getZOrder();
            });
        _needsResort = false;
    }

    for (const auto& character : _sortedCharacters) {
        auto it = std::find_if(visible.begin(), visible.end(),
            [&](const VisibleChar& vc) { return vc.character == character; });

        if (it != visible.end()) {
            Sprite* sprite = character->getSprite();
            sprite->drawTo(_buffer, it->screenX, it->screenY);
        }
    }

    _buffer.pushSprite(0, 0);
}

void Display::setMac(uint32_t mac) {
    myMac = mac;
}