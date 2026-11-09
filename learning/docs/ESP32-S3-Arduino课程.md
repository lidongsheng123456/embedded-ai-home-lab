# ESP32-S3 Arduino Core 课程

这份文件是 ESP32-S3 学习的唯一入口。完成 STM32 8章后再开始本课程，继续使用 PlatformIO 和 Arduino Core，底层 FreeRTOS 已由框架提供。

## 0. 开始前

VS Code 直接打开 `learning/examples/esp32-s3`。在 `Project Tasks → General` 使用 Build/Upload，或在终端指定环境：

```powershell
pio run -e 01a_gpio_blink
pio run -e 01a_gpio_blink -t upload
```

所有公共引脚集中在 [`board_config.h`](../examples/esp32-s3/include/board_config.h)。不同ESP32-S3板卡的板载灯、RGB灯和PSRAM不一定相同，必须核对自己的板卡原理图。GPIO只允许3.3V；电机、灯带和喇叭不能由GPIO直接供电。

## 1. GPIO输入输出与WS2812

案例：`01a_gpio_blink` 普通灯闪烁、`01b_gpio_button` 按键控灯、`01c_ws2812_colors` 彩灯三色循环。

普通LED接 GPIO2→220Ω电阻→LED正极，负极接GND；按键接GPIO4与GND；WS2812数据线默认GPIO48。`pinMode` 配置输入输出，`digitalWrite` 输出电平，`INPUT_PULLUP` 使按键松开为高、按下为低。`rgbLedWrite` 底层利用RMT产生WS2812所需的精确脉冲。

LED不亮先检查方向和引脚；灯带需要外部供电并共地。能完成三个现象并解释高低电平即可过关，对应状态灯、实体按键和彩色眼睛。

## 2. 系统时间与延时

案例：`02a_task_delay` 使用FreeRTOS任务延时闪灯；`02b_uptime` 每秒打印开机时间戳。

`vTaskDelay(pdMS_TO_TICKS(500))` 让当前任务休息并把CPU交给其他任务；`millis()` 返回开机后的毫秒数，不是真实日期。长期计时应使用无符号减法处理溢出。能区别 `delay`、`vTaskDelay` 和 `millis` 即可过关，对应周期采样和界面刷新。

## 3. GPIO外部中断

案例：`03a_gpio_interrupt` 按键下降沿切灯；`03b_interrupt_debounce` 增加50ms消抖。按键仍接GPIO4与GND。

`attachInterrupt(..., FALLING)` 在高变低时调用中断函数。中断里只设置事件标记，不打印、不联网、不长延时；主循环再处理灯光。共享标记使用 `volatile`。一次按键只切换一次即过关，对应触摸和桌宠互动。

## 4. UART串口

案例：`04a_uart_log` 输出运行日志；`04b_uart_command` 接收 `ON/OFF/TOGGLE` 控灯。

USB串口波特率115200。`Serial.printf` 输出带变量的文字；`readStringUntil('\n')` 接收一行，串口工具发送结尾应选择换行。外接串口模块必须3.3V电平、TX/RX交叉并共地。三条命令均有效且未知命令不死机即过关。

## 5. PWM与RMT

案例：`05a_pwm_fade` 用LEDC让GPIO2 LED呼吸；`05b_ws2812_animation` 非阻塞切换RGB颜色。

PWM是快速开关形成平均亮度。`ledcAttach` 配置5kHz和8位分辨率，`ledcWrite` 的0～255改变占空比。彩灯案例不用长延时，而用 `millis()` 判断换色时间，让主循环还能处理其他事情。能区别频率与占空比即过关。

## 6. ADC模拟采集

案例：`06a_adc_potentiometer` 读取GPIO1电位器；`06b_adc_light` 读取GPIO2光敏分压；`06c_adc_battery` 读取GPIO3电池分压。

ADC像电压尺子，12位结果范围0～4095，电压估算为 `raw×3.3/4095`。电池必须先经电阻分压，任何GPIO都不能直接输入5V。阈值和分压比例需要根据实物校准。能同时报告原始值和工程值即过关。

## 7. I2C与SPI显示

案例：`07a_i2c_oled` 驱动SSD1306，默认SDA=8、SCL=9、地址0x3C；`07b_spi_tft` 驱动ILI9341，默认SCK=12、MISO=13、MOSI=11、CS=10、DC=6、RST=7。

I2C像双线公交，用地址找设备；SPI像高速专线，用CS片选设备。OLED无显示先核对0x3C/0x3D和供电；TFT白屏先核对驱动芯片、CS/DC/RST。首次构建会自动下载Adafruit显示库。两块屏显示指定文字即过关。

## 8. Wi-Fi联网

案例：`08a_wifi_time` 连接路由器并通过NTP获取北京时间；`08b_wifi_web_led` 让手机浏览器控制LED。

复制 `include/wifi_config.example.h` 为 `include/wifi_config.h` 后填写网络名称和密码。真实配置已被Git忽略。ESP32-S3通常连接2.4GHz Wi-Fi；手机与开发板需处于同一局域网。`WiFi.begin` 使用STA模式，`WebServer` 在80端口处理请求。手机能打开网页并控制灯即过关。

## 9. BLE蓝牙

案例：`09a_ble_led` 向BLE特征值写 `ON/OFF`；`09b_ble_buzzer` 写 `BEEP` 触发有源蜂鸣器。

BLE像手机附近的小型数据服务。Service UUID表示一组服务，Characteristic UUID表示其中的数据入口。手机要使用BLE GATT调试工具，不是只在系统设置中配对。蜂鸣器信号默认GPIO5；大功率蜂鸣器/喇叭需三极管或功放。无需路由器即可控制即过关。

## 10. FreeRTOS多任务

案例：`10a_dual_tasks` 让闪灯与日志任务并行；`10b_queue_tasks` 让采集任务通过队列把ADC数据交给打印任务。

任务像轮流使用CPU的工作人员。`xTaskCreate` 创建任务，任务循环必须适当 `vTaskDelay`，否则会占满CPU并触发看门狗。Queue（队列）像线程安全传送带，避免多个任务抢同一个全局变量。能画出“采集任务→队列→显示任务”即过关。

## 11. PSRAM、I2S与摄像头

案例：`11a_psram_buffer` 尝试在PSRAM申请320×240缓冲；`11b_i2s_tone` 通过MAX98357A播放测试音；`11c_camera_preflight` 检查CPU核心与PSRAM容量。

PSRAM是给图片和音频使用的外部大内存，N8通常没有PSRAM，带R8的模组才常见8MB PSRAM。I2S默认BCLK=16、LRC=17、DIN=18，喇叭必须接功放输出，不能直接接GPIO。

摄像头没有通用引脚表。必须先确定具体模组，再按原理图配置D0-D7、XCLK、PCLK、VSYNC、HREF、SDA、SCL。预检案例故意不写万能初始化，以免错误接线。能报告PSRAM容量并解释I2S三根主要信号即过关。

## 与HomeGuard Mini的关系

GPIO、ADC、I2C/SPI负责传感器和屏幕；Wi-Fi/BLE负责联网配置；FreeRTOS负责并行采集、显示和通信；PSRAM/I2S为动画与音效提供基础。完成课程后，从 [`projects/homeguard-mini`](../../projects/homeguard-mini/) 的 `app / drivers / services / domain / ui` 分层开始阅读成品代码。
