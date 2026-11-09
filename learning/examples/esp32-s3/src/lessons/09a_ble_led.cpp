/* 第9章案例A：手机向BLE特征值写入ON/OFF控制LED。 */
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "board_config.h"

constexpr char SERVICE_UUID[] = "7d2a0001-8f77-4c8b-a754-2f66e8420a01";
constexpr char COMMAND_UUID[] = "7d2a0002-8f77-4c8b-a754-2f66e8420a01";

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String command = characteristic->getValue();
    command.trim();
    command.toUpperCase();
    if (command == "ON") digitalWrite(STATUS_LED_PIN, HIGH);
    if (command == "OFF") digitalWrite(STATUS_LED_PIN, LOW);
  }
};

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  BLEDevice::init("HomeGuard-Learn");
  BLEServer *server = BLEDevice::createServer();
  BLEService *service = server->createService(SERVICE_UUID);
  BLECharacteristic *command = service->createCharacteristic(COMMAND_UUID, BLECharacteristic::PROPERTY_WRITE);
  command->setCallbacks(new CommandCallbacks());
  service->start();
  BLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
  BLEDevice::getAdvertising()->start();
}

void loop() {
  delay(100);
}
