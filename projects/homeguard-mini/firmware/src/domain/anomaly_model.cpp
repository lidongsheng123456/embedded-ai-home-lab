#include "domain/anomaly_model.h"

#include <math.h>

void AnomalyModel::begin() {
  // v3改用周期内声音峰值，不能沿用v2单个声音窗口的统计基线。
  preferences_.begin("homeguard-ai3", false);
  count_ = preferences_.getUInt("count", 0);
  if (count_ > 0) {
    if (preferences_.getBytesLength("mean") == sizeof(mean_)) {
      preferences_.getBytes("mean", mean_, sizeof(mean_));
    } else {
      reset();
      return;
    }
    if (preferences_.getBytesLength("m2") == sizeof(m2_)) {
      preferences_.getBytes("m2", m2_, sizeof(m2_));
    } else {
      reset();
    }
  }
}

void AnomalyModel::reset() {
  count_ = 0;
  memset(mean_, 0, sizeof(mean_));
  memset(m2_, 0, sizeof(m2_));
  preferences_.clear();
}

void AnomalyModel::learn(const float features[FEATURE_COUNT]) {
  ++count_;
  for (size_t i = 0; i < FEATURE_COUNT; ++i) {
    // 使用在线算法更新均值和方差：每次只处理新样本，不必把历史数据全存进内存。
    const float delta = features[i] - mean_[i];
    mean_[i] += delta / static_cast<float>(count_);
    const float delta2 = features[i] - mean_[i];
    m2_[i] += delta * delta2;
  }
  // 不每次采样都写Flash，避免频繁擦写缩短存储寿命。
  if (count_ % 12 == 0) save();
}

float AnomalyModel::score(const float features[FEATURE_COUNT]) const {
  if (!trained() || count_ < 2) return 0.0f;
  float squaredSum = 0.0f;
  for (size_t i = 0; i < FEATURE_COUNT; ++i) {
    const float variance = m2_[i] / static_cast<float>(count_ - 1);
    // 方差下限可防止几乎不变的特征产生无限大的分数。
    const float stddev = sqrtf(fmaxf(variance, 0.0001f));
    // z表示“当前值离平时均值有多少个标准差”；越大说明越反常。
    const float z = (features[i] - mean_[i]) / stddev;
    // 单个失控传感器最多贡献100，避免它完全淹没其他特征。
    squaredSum += fminf(z * z, 100.0f);
  }
  // 合并五种特征，得到没有物理单位的综合异常分数。
  return sqrtf(squaredSum / FEATURE_COUNT);
}

void AnomalyModel::save() {
  preferences_.putUInt("count", count_);
  preferences_.putBytes("mean", mean_, sizeof(mean_));
  preferences_.putBytes("m2", m2_, sizeof(m2_));
}
