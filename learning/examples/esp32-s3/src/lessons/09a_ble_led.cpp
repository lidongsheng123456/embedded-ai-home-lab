/*
 * 第 9 章 / 案例 A：手机通过BLE控制LED
 * 板卡：ESP32-S3 DevKitC N16R8；芯片自带BLE，不连接外置蓝牙模块。
 * 接线：GPIO2 -> 限流电阻 -> LED -> GND。
 * 操作：手机BLE调试工具连接HomeGuard-Learn，向命令特征写入ON或OFF。
 * 现象：写入ON时LED亮，写入OFF时LED灭；无需路由器或互联网。
 */
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "board_config.h"

// UUID相当于BLE功能的唯一编号；手机根据它找到本案例的服务和命令入口。
constexpr char SERVICE_UUID[] = "7d2a0001-8f77-4c8b-a754-2f66e8420a01";
constexpr char COMMAND_UUID[] = "7d2a0002-8f77-4c8b-a754-2f66e8420a01";

// 手机写入Characteristic（特征值）时，BLE库会调用这个回调函数。
class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    // 统一去掉首尾空白并转成大写，让“on”和“ON”都能识别。
    String command = characteristic->getValue().c_str();
    command.trim();
    command.toUpperCase();
    if (command == "ON") digitalWrite(STATUS_LED_PIN, HIGH);
    if (command == "OFF") digitalWrite(STATUS_LED_PIN, LOW);
  }
};

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);

  // 初始化BLE协议栈，并设置手机扫描时看到的设备名称。
  BLEDevice::init("HomeGuard-Learn");
  BLEServer *server = BLEDevice::createServer();

  // Service像功能文件夹，Characteristic像其中可写入命令的数据入口。
  BLEService *service = server->createService(SERVICE_UUID);
  BLECharacteristic *command = service->createCharacteristic(COMMAND_UUID, BLECharacteristic::PROPERTY_WRITE);
  command->setCallbacks(new CommandCallbacks());
  service->start();

  // 广播服务UUID后，手机才能扫描并发现本案例提供的功能。
  BLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
  BLEDevice::getAdvertising()->start();
}

void loop() {
  // BLE收发由协议栈在后台处理，主循环无需主动轮询特征值。
  delay(100);
}
