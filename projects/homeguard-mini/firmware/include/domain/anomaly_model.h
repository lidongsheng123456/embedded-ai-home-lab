#pragma once

#include <Arduino.h>
#include <Preferences.h>

// 在设备本地学习“平时是什么样”，并计算当前环境偏离平时的程度。
// 它只处理数字和模型存储，不读取传感器，也不直接触发蜂鸣器。
class AnomalyModel {
 public:
  static constexpr size_t FEATURE_COUNT = 5;  // 气体、温度、湿度、光照、声音。
  static constexpr uint32_t MIN_TRAINING_SAMPLES = 120;  // 约10分钟数据。

  // 打开ESP32的持久化存储，并恢复上一次保存的学习结果。
  void begin();
  // 清空内存和Flash中的旧基线，让设备重新学习当前房间。
  void reset();
  // 加入一组正常环境特征，逐步更新均值和波动范围。
  void learn(const float features[FEATURE_COUNT]);
  // 返回当前特征偏离正常基线的程度；模型未训练完成时返回0。
  float score(const float features[FEATURE_COUNT]) const;
  // 样本达到最低数量以后，才允许模型参与异常判断。
  bool trained() const { return count_ >= MIN_TRAINING_SAMPLES; }
  // 供屏幕和网页显示当前已经学习了多少组数据。
  uint32_t samples() const { return count_; }
  // 把当前模型写入Flash，避免每次断电都从零开始。
  void save();

 private:
  Preferences preferences_;  // ESP32提供的键值存储对象。
  uint32_t count_ = 0;       // 已学习的样本总数。
  float mean_[FEATURE_COUNT] = {0};  // 每种特征的在线平均值。
  float m2_[FEATURE_COUNT] = {0};    // 用于计算每种特征的方差。
};
