#include <Arduino.h>

#include "pins.h"

#ifndef PET_STAGE
#define PET_STAGE 1
#endif

#if PET_STAGE >= 2
#include <U8g2lib.h>
#include <Wire.h>
#endif

#if PET_STAGE >= 3
#include <Servo.h>
#endif

namespace {

enum class Mood : uint8_t { Neutral, Happy, Sad, Sleepy };

Mood mood = Mood::Happy;
uint32_t lastBoardLedAt = 0;
bool boardLedOn = false;

void setBoardLed(bool on) {
  boardLedOn = on;
  digitalWrite(PIN_BOARD_LED, on ? LOW : HIGH);
}

void updateBoardLed(uint32_t now) {
  if (now - lastBoardLedAt < 500) return;
  lastBoardLedAt = now;
  setBoardLed(!boardLedOn);
}

#if PET_STAGE >= 2

// 套装中的 1.3 英寸 OLED 通常使用 SH1106。
// 如果屏幕确认是 SSD1306，请按 README 修改这一行。
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

bool eyesClosed = false;
uint32_t lastBlinkAt = 0;
uint32_t eyesClosedAt = 0;

void drawFace() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(2, 9, "STM32 DESK PET");

  if (eyesClosed) {
    display.drawHLine(24, 27, 22);
    display.drawHLine(82, 27, 22);
  } else if (mood == Mood::Sad) {
    display.drawLine(24, 24, 45, 29);
    display.drawLine(83, 29, 104, 24);
  } else if (mood == Mood::Sleepy) {
    display.drawHLine(24, 28, 22);
    display.drawHLine(82, 28, 22);
  } else {
    display.drawDisc(35, 27, 5);
    display.drawDisc(93, 27, 5);
  }

  if (mood == Mood::Happy) {
    display.drawLine(45, 45, 54, 52);
    display.drawHLine(54, 52, 20);
    display.drawLine(74, 52, 83, 45);
  } else if (mood == Mood::Sad) {
    display.drawLine(45, 53, 54, 46);
    display.drawHLine(54, 46, 20);
    display.drawLine(74, 46, 83, 53);
  } else if (mood == Mood::Sleepy) {
    display.drawStr(58, 51, "zZ");
  } else {
    display.drawHLine(50, 49, 28);
  }

  display.setCursor(2, 63);
  display.print("STEP ");
  display.print(PET_STAGE);
  display.sendBuffer();
}

void setMood(Mood nextMood) {
  mood = nextMood;
  drawFace();
}

void updateEyeBlink(uint32_t now) {
  if (!eyesClosed && now - lastBlinkAt >= 3000) {
    eyesClosed = true;
    eyesClosedAt = now;
    lastBlinkAt = now;
    drawFace();
  } else if (eyesClosed && now - eyesClosedAt >= 160) {
    eyesClosed = false;
    drawFace();
  }
}

#endif

#if PET_STAGE >= 3

Servo headServo;
uint8_t headAngle = 70;
uint32_t lastHeadMoveAt = 0;

void setHeadAngle(int angle) {
  headAngle = static_cast<uint8_t>(constrain(angle, 45, 135));
  headServo.write(headAngle);
}

void updateAutomaticHead(uint32_t now) {
#if PET_STAGE < 5
  if (now - lastHeadMoveAt < 1200) return;
  lastHeadMoveAt = now;
  setHeadAngle(headAngle < 90 ? 115 : 65);
#else
  (void)now;
#endif
}

#endif

#if PET_STAGE >= 4

bool lastButtonState = HIGH;
uint32_t lastButtonAt = 0;

void cycleMood() {
  switch (mood) {
    case Mood::Neutral: setMood(Mood::Happy); break;
    case Mood::Happy: setMood(Mood::Sad); break;
    case Mood::Sad: setMood(Mood::Sleepy); break;
    case Mood::Sleepy: setMood(Mood::Neutral); break;
  }
  digitalWrite(PIN_STATUS_LED, mood == Mood::Happy ? HIGH : LOW);
}

void updateMoodButton(uint32_t now) {
  const bool state = digitalRead(PIN_MOOD_BUTTON);
  if (lastButtonState == HIGH && state == LOW && now - lastButtonAt >= 250) {
    lastButtonAt = now;
    cycleMood();
  }
  lastButtonState = state;
}

#endif

#if PET_STAGE >= 5

HardwareSerial bluetooth(PIN_BLUETOOTH_RX, PIN_BLUETOOTH_TX);

void handleBluetoothCommand(char command) {
  switch (command) {
    case 'L': case 'l': setHeadAngle(55); break;
    case 'R': case 'r': setHeadAngle(125); break;
    case 'C': case 'c': setHeadAngle(90); break;
    case 'H': case 'h': setMood(Mood::Happy); break;
    case 'S': case 's': setMood(Mood::Sad); break;
    case 'N': case 'n': setMood(Mood::Neutral); break;
    case 'Z': case 'z': setMood(Mood::Sleepy); break;
    default: return;
  }
  digitalWrite(PIN_STATUS_LED, mood == Mood::Happy ? HIGH : LOW);
}

void updateBluetooth() {
  while (bluetooth.available() > 0) {
    handleBluetoothCommand(static_cast<char>(bluetooth.read()));
  }
}

#endif

}  // namespace

void setup() {
  pinMode(PIN_BOARD_LED, OUTPUT);
  setBoardLed(false);

#if PET_STAGE >= 2
  Wire.setSDA(PIN_OLED_SDA);
  Wire.setSCL(PIN_OLED_SCL);
  Wire.begin();
  display.setI2CAddress(OLED_I2C_ADDRESS << 1);
  display.begin();
  drawFace();
#endif

#if PET_STAGE >= 3
  headServo.attach(PIN_HEAD_SERVO);
  setHeadAngle(90);
#endif

#if PET_STAGE >= 4
  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_MOOD_BUTTON, INPUT_PULLUP);
  digitalWrite(PIN_STATUS_LED, HIGH);
#endif

#if PET_STAGE >= 5
  bluetooth.begin(9600);
#endif
}

void loop() {
  const uint32_t now = millis();
  updateBoardLed(now);

#if PET_STAGE >= 2
  updateEyeBlink(now);
#endif

#if PET_STAGE >= 3
  updateAutomaticHead(now);
#endif

#if PET_STAGE >= 4
  updateMoodButton(now);
#endif

#if PET_STAGE >= 5
  updateBluetooth();
#endif
}

