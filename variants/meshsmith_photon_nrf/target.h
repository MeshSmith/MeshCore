#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <PhotonNrf52Board.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <helpers/SensorManager.h>
#include "MeshsmithSensorManager.h"

extern PhotonNrf52Board board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern MeshsmithSensorManager sensors;

bool radio_init();
mesh::LocalIdentity radio_new_identity();
