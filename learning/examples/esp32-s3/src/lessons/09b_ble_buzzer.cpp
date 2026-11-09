/*
 * 第 9 章 / 案例 B：手机通过BLE触发有源蜂鸣器
 * 板卡：ESP32-S3 DevKitC N16R8；使用芯片内置BLE。
 * 接线：GPIO5通过合适的三极管驱动电路控制有源蜂鸣器，所有电源必须共地。
 * 操作：手机连接HomeGuard-Buzzer，向命令特征写入BEEP。
 * 现象：每次收到命令，蜂鸣器响约150 ms。
 * 限制：GPIO不能直接驱动喇叭、无源大蜂鸣器或其他大电流负载。
 */
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "board_config.h"

constexpr char SERVICE_UUID[] = "7d2a0011-8f77-4c8b-a754-2f66e8420a01";
constexpr char COMMAND_UUID[] = "7d2a0012-8f77-4c8b-a754-2f66e8420a01";

// 回调和主循环可能在不同执行上下文访问该标志，因此使用volatile。
volatile bool beep_requested = false;

class BeepCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String command = characteristic->getValue().c_str();
    command.trim();
    command.toUpperCase();
    // BLE回调只记录请求，不在协议栈回调中执行delay等耗时动作。
    if (command == "BEEP") beep_requested = true;
  }
};

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  // GPIO5输出HIGH时驱动电路开启蜂鸣器，LOW时关闭。
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
    // 主循环接手耗时动作：清除请求、拉高150 ms，再关闭蜂鸣器。
    beep_requested = false;
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
  }
  delay(10);
}
