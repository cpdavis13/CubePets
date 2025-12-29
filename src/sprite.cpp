#include "sprite.h"
#include "assets/warrior.h"

Sprite::Sprite(TFT_eSPI* tft)
  : _tft(tft),
    _frames(0),
    _totalFrames(0),
    _frameWidth(0),
    _frameHeight(0),
    _transparentColor(TFT_WHITE),
    _currentAnim(nullptr),
    _currentFrame(0)
{
  _sheetWidth = _frameWidth * _totalFrames; 
}

void Sprite::load(int id) {
  switch (id) {
    case 0: {
      _frames = warrior;
      _totalFrames = 10;
      _frameWidth = 39;
      _frameHeight = 47;
      _sheetWidth = _frameWidth * _totalFrames;

      uint8_t idle[] = { 0, 1, 2, 3, 4, 5 };
      addAnimation("idle", idle, 6, 100, true); // looping

      uint8_t death[] = { 6, 7, 8, 9 };
      addAnimation("death", death, 4, 100, false); // non-looping

      break;
    }
    default:
      break;
  }
}

void Sprite::addAnimation(const String& name, const uint8_t* indices, uint8_t count, uint16_t frameDuration, bool loop) {
  uint8_t* frameCopy = new uint8_t[count];
  memcpy(frameCopy, indices, count);
  Animation anim = { frameCopy, count, 0, frameDuration, millis(), loop };
  _animations[name] = anim;
}

void Sprite::play(const String& name, bool loop) {
  auto it = _animations.find(name);
  if (it != _animations.end()) {
    _currentAnim = &it->second;
    _currentAnim->currentFrame = 0;
    _currentAnim->lastFrameTime = millis();
    _currentAnim->loop = loop; // Update loop in case it changes dynamically
    _currentFrame = _currentAnim->frameIndices[0];
  }
}

void Sprite::update() {
  if (_currentAnim) {
    unsigned long now = millis();
    if (now - _currentAnim->lastFrameTime >= _currentAnim->frameDuration) {
      if (_currentAnim->currentFrame < _currentAnim->frameCount - 1) {
        _currentAnim->currentFrame++;
      } else if (_currentAnim->loop) {
        _currentAnim->currentFrame = 0;
      } else {
        // Stay on last frame
      }
      _currentFrame = _currentAnim->frameIndices[_currentAnim->currentFrame];
      _currentAnim->lastFrameTime = now;
    }
  }
}

void Sprite::drawTo(TFT_eSprite& buffer, int16_t x, int16_t y) {
  uint16_t framesPerRow = _sheetWidth / _frameWidth;
  uint16_t frameX = (_currentFrame % framesPerRow) * _frameWidth;
  uint16_t frameY = (_currentFrame / framesPerRow) * _frameHeight;

  for (uint16_t py = 0; py < _frameHeight; py++) {
    for (uint16_t px = 0; px < _frameWidth; px++) {
      uint32_t index = (frameY + py) * _sheetWidth + (frameX + px);
      uint16_t pixel = _frames[index];
      if (pixel != _transparentColor) {
        buffer.drawPixel(x + px, y + py, pixel);
      }
    }
  }
}

uint8_t Sprite::getFrame() { return _currentFrame; }

void Sprite::setFrame(uint8_t frame) {
  _currentAnim = nullptr;
  _currentFrame = frame;
}

uint16_t Sprite::getFrameWidth() { return _frameWidth; }
uint16_t Sprite::getFrameHeight() { return _frameHeight; }
