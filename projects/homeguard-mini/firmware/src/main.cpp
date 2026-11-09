#include <Arduino.h>

#include "app/homeguard_app.h"

// Arduino 框架仍需要 setup/loop；具体职责由 HomeGuardApp 分发给各模块。
HomeGuardApp app;

void setup() { app.setup(); }

void loop() { app.loop(); }
