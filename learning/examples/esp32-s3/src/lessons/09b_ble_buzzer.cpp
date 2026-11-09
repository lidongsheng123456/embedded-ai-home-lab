/* 第9章案例B：手机写入BEEP触发有源蜂鸣器；GPIO不能直接带大功率喇叭。 */
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "board_config.h"

constexpr char SERVICE_UUID[] = "7d2a0011-8f77-4c8b-a754-2f66e8420a01";
constexpr char COMMAND_UUID[] = "7d2a0012-8f77-4c8b-a754-2f66e8420a01";
volatile bool beep_requested = false;

class BeepCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String command = characteristic->getValue();
    command.trim();
    command.toUpperCase();
    if (command == "BEEP") beep_requested = true;
  }
};

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  BLEDevice::init("HomeGuard-Buzzer");
  BLEService *service = BLEDevice::createServer()->createService(SERVICE_UUID);
  BLECharacteristic *command = service->createCharacteristic(COMMAND_UUID, BLECharacteristic::PROPERTY_WRITE);
  command->setCallbacks(new BeepCallbacks());
  service->start();
  BLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
  BLEDevice::getAdvertising()->start();
}

void loop() {
  if (beep_requested) {
    beep_requested = false;
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
  }
  delay(10);
}
