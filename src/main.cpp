#include "battle.h"
#include "TFT_eSPI.h"

TFT_eSPI tft;
Battle battle(&tft, 128, 128);



void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Setup");
  delay(100);
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLUE);

  battle.init();
}



void loop() {
  battle.update();
}

