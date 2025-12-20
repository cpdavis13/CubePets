#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <map>

class Sprite {
public:
  Sprite(TFT_eSPI* tft);
  void load(int id);
  void addAnimation(const String& name, const uint8_t* indices, uint8_t count, uint16_t frameDuration, bool loop);
  void play(const String& name, bool loop = true);
  void update();
  void drawTo(TFT_eSprite& buffer, int16_t x, int16_t y);

  uint8_t getFrame();
  void setFrame(uint8_t frame);
  uint16_t getFrameWidth();
  uint16_t getFrameHeight(); 

private:
  TFT_eSPI* _tft;
  const uint16_t* _frames; 
  uint8_t _totalFrames;

  uint16_t _frameWidth;
  uint16_t _frameHeight;
  uint16_t _sheetWidth;
  uint16_t _transparentColor;

  uint8_t _currentFrame = 0;

  struct Animation {
    uint8_t* frameIndices;
    uint8_t frameCount;
    uint8_t currentFrame;
    uint16_t frameDuration;
    unsigned long lastFrameTime;
    bool loop;
  };

  std::map<String, Animation> _animations;
  Animation* _currentAnim = nullptr;
};
