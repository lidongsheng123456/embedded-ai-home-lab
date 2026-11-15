#include "ui/display_view.h"

#include <math.h>

namespace {
String numberOrUnavailable(float value, uint8_t decimals, const char *unit = "") {
  return isfinite(value)
             ? String(value, static_cast<unsigned int>(decimals)) + unit
             : String("暂无数据");
}

const char *statusLabel(const String &state) {
  // 只翻译领域层发布的状态，不在界面重新判断告警条件。
  static const struct {
    const char *state;
    const char *label;
  } labels[] = {{"Starting", "正在启动"}, {"Normal", "环境正常"},
                {"Learning", "正在学习"}, {"High risk", "环境变化较大"},
                {"Attention", "请留意变化"}, {"Sensor warning", "传感器待检查"},
                {"Warming up", "传感器预热中"}};
  for (const auto &entry : labels) {
    if (state == entry.state) return entry.label;
  }
  return "状态未知";
}
}  // namespace

void DisplayView::begin() {
  tft_.init();
  tft_.setRotation(0);
  tft_.fillScreen(TFT_BLACK);
  text_.begin(tft_);
  // 16像素文泉驿字库存于Flash，支持简体中文，不需要上传文件系统。
  text_.setFont(u8g2_font_wqy16_t_gb2312);
  text_.setFontMode(1);
  text_.setForegroundColor(TFT_WHITE);
  drawCenteredText("居家环境助手", 78);
  drawCenteredText("正在启动传感器…", 116);
}

void DisplayView::drawText(const String &text, int x, int top) {
  // 字库使用基线坐标，统一转成顶部坐标，避免汉字与数字上下错位。
  text_.drawUTF8(x, top + text_.getFontAscent(), text.c_str());
}

void DisplayView::drawCenteredText(const String &text, int top) {
  drawText(text, (tft_.width() - text_.getUTF8Width(text.c_str())) / 2, top);
}

void DisplayView::render(const HomeState &state, const IPAddress &deviceIp) {
  // 红、橙、绿只负责表达领域层已经算好的等级，UI本身不判断风险。
  const uint16_t color =
      state.level == 2 ? TFT_RED : state.level == 1 ? TFT_ORANGE : TFT_GREEN;
  tft_.fillScreen(TFT_BLACK);
  tft_.fillRect(0, 0, tft_.width(), 34, color);
  text_.setForegroundColor(TFT_BLACK);
  drawText(statusLabel(state.state), 8, 9);
  if (state.cameraReady) drawText("相机", tft_.width() - 40, 9);

  const struct {
    const char *label;
    String value;
  } rows[] = {
      {"气体指数", numberOrUnavailable(state.gasIndex, 0, " /100")},
      {"温度", numberOrUnavailable(state.temperature, 1, " ℃")},
      {"湿度", numberOrUnavailable(state.humidity, 1, " %")},
      {"光照", numberOrUnavailable(state.light, 0, " lx")},
      {"声音指数", numberOrUnavailable(state.sound, 0)},
      {"人体检测", !state.radarValid ? "未知" : state.presence ? "有人" : "无人"},
      {"异常评分", numberOrUnavailable(state.anomalyScore, 2)}};
  text_.setForegroundColor(TFT_WHITE);
  const int firstY = 42;
  const int rowHeight = 24;
  for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
    const int top = firstY + i * rowHeight;
    drawText(rows[i].label, 8, top);
    drawText(rows[i].value, 104, top);
  }
  text_.setForegroundColor(TFT_CYAN);
  drawCenteredText(deviceIp.toString(), tft_.height() - 18);
}
