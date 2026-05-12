#include <Arduino.h>

#include "MeshsmithPhotonC6Board.h"

void MeshsmithPhotonC6Board::begin() {
  ESP32Board::begin();

#ifdef USE_XIAO_ESP32C6_EXTERNAL_ANTENNA
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);

  delay(100);

  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);
#endif

  delay(10);   // give sx1262 some time to power up
}
