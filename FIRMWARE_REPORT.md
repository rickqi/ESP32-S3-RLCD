# ESP32-S3-RLCD-4.2 固件分析报告

> 生成日期：2026-07-30 | 设备 MAC：A4:CB:8F:DA:8C:9C | 端口：COM4

---

## 1. 设备硬件信息

| 项目 | 值 |
|------|------|
| 芯片型号 | ESP32-S3 (QFN56), revision v0.2 |
| 模组 | ESP32-S3-WROOM-1-N16R8 |
| Flash | 16MB |
| PSRAM | 8MB (Embedded, Octal PSRAM) |
| MAC 地址 | `A4:CB:8F:DA:8C:9C` |
| USB 模式 | USB-Serial/JTAG |
| 晶振 | 40MHz |
| 特性 | Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240MHz |

---

## 2. 板载按钮

| 按钮 | GPIO | 功能 | 来源 |
|------|------|------|------|
| **BOOT** | GPIO0 | 按住BOOT+重新上电 → 进入下载/烧录模式 | 官方文档 + 源码 config.h |
| **PWR** | — | 电源键：单击上电，长按下电 | 官方文档 |
| **KEY** | GPIO18 | 用户自定义按键 | 官方文档 + 源码 config.h |

> **注意**：此板没有传统 RST/RESET 按钮。PWR 按钮兼做电源控制。如需复位，长按 PWR 关机再单击开机，或拔插 USB。

---

## 3. 设备原有固件检测（烧录前）

### 3.1 检测方法

通过 `esptool read-flash` 读取 Flash 0x8000 处的分区表和 0x10000 处的应用程序描述符（esp_app_desc_t），解析固件元数据。

### 3.2 原有固件信息

| 项目 | 值 |
|------|------|
| **项目名** | `arduino-lib-builder` |
| **版本** | `43a8f6d`（git commit hash） |
| **编译日期** | Jun 2 2026, 11:17:54 |
| **ESP-IDF 版本** | v5.5.4 |
| **功能** | WiFi 热点（AP 模式） |
| **SSID** | `ESP32_AP` |
| **密码** | `12345678` |

### 3.3 原有分区表

```
nvs:      0x009000, 20KB
otadata:  0x00E000, 8KB
ota_0:    0x010000, 1.25MB  ← 原有活动分区
ota_1:    0x150000, 1.25MB
spiffs:   0x290000, 1.38MB
coredump: 0x3F0000, 64KB
```

> **结论**：这不是出厂固件，也不是小智 AI 固件。是某个通过 Arduino IDE 编译的自定义 WiFi AP 测试程序。分区表也跟微雪官方的两套都不一致。

---

## 4. 预编译固件清单（本地已有）

路径前缀：`docs/examples/ESP32-S3-RLCD-4.2-GitHub/03_Firmware/`

| # | 文件名 | 大小 | 字节数 | 说明 |
|---|--------|------|--------|------|
| 1 | `01_Factory_V1.bin` | 4.34 MB | 4,548,144 | 出厂综合测试程序 |
| 2 | `02_XiaoZhi_V2.1.0.bin` | 10.66 MB | 11,179,471 | 小智 AI 语音助手 V2.1.0 |

### 4.1 出厂测试固件（01_Factory_V1.bin）

| 项目 | 值 |
|------|------|
| 项目名 | `03_Fac`（FactoryProgram） |
| 版本 | 1 |
| 编译日期 | Jan 20 2026, 18:13:54 |
| ESP-IDF | v5.5.2 |
| 分区表 | 单 factory 分区（nvs 24KB + phy_init 4KB + factory 8M） |
| WiFi | 硬编码：SSID=`PDCN`，密码=`1234567890` |
| 功能 | 屏幕显示 + WiFi扫描计数 + 温湿度 + RTC + 电池 + 音频 |

**WiFi 实现**（`components/app_bsp/esp_wifi_bsp.c`）：
- WiFi 工作模式：STA（站点模式）
- SSID 和密码在源码中硬编码，无配网 UI
- 启动后自动扫描周围 WiFi 热点，显示数量
- 无 SmartConfig / BluFi / Web provisioning

### 4.2 小智 AI 固件（02_XiaoZhi_V2.1.0.bin）

| 项目 | 值 |
|------|------|
| 版本 | V2.1.0 |
| 分区表 | v2/16m.csv（dual-OTA + 8MB assets） |
| 功能 | AI 语音聊天助手（ASR + LLM + TTS） |
| WiFi | 支持扫码配网、网页配网（xiaozhi.me） |

---

## 5. 可编译固件源码清单

### 5.1 ESP-IDF 示例（11 个）

路径：`02_Example/ESP-IDF/`

| # | 示例 | Flash | 自定义分区 | BT | IDF版本 | 说明 |
|---|------|-------|-----------|-----|---------|------|
| 01 | WIFI_AP | 8MB | 否 | 否 | 5.5.1 | WiFi 热点 |
| 02 | WIFI_STA | 16MB | 否 | 否 | 5.4.0 | WiFi 连接 |
| 03 | ADC_Test | 16MB | 是 | 是 | 5.5.1 | 电池电压 |
| 04 | I2C_PCF85063 | 16MB | 是 | 否 | 5.5.1 | RTC 时钟 |
| 05 | I2C_SHTC3 | 16MB | 是 | 否 | 5.5.1 | 温湿度 |
| 06 | SD_Card | 16MB | 是 | 否 | 5.5.1 | TF 卡 |
| 07 | Audio_Test | 16MB | 是 | 否 | 5.5.1 | ES8311/ES7210 |
| 08 | LVGL_V8_Test | 16MB | 是 | 否 | 5.5.1 | LVGL v8 GUI |
| 09 | LVGL_V9_Test | 16MB | 是 | 否 | 5.5.1 | LVGL v9 GUI |
| 10 | FactoryProgram | 16MB | 是 | 是 | 5.5.1 | 出厂综合测试 |
| 11 | U8G2_Test | 16MB | 是 | 否 | 5.5.1 | U8g2 图形库 |

### 5.2 ESP-IDF 分区表（示例 03-11 共用）

```
nvs,      data, nvs,     auto, 0x6000  (24 KB)
phy_init, data, phy,     auto, 0x1000  (4 KB)
factory,  app,  factory, auto, 8M
```

### 5.3 XiaoZhi AI 分区表（v2/16m.csv）

```
nvs,      data, nvs,     0x9000,   0x4000   (16 KB)
otadata,  data, ota,     0xd000,   0x2000   (8 KB)
phy_init, data, phy,     0xf000,   0x1000   (4 KB)
ota_0,    app,  ota_0,   0x20000,  0x3f0000 (~4 MB)
ota_1,    app,  ota_1,   auto,     0x3f0000 (~4 MB)
assets,   data, spiffs,  0x800000, 8M        (8 MB)
```

### 5.4 ESPHome 配置（3 个 YAML）

路径：`02_Example/ESPHome/examples/`

| 配置 | 功能 |
|------|------|
| `esp32-s3-rlcd-42-sensor.yaml` | 屏幕 + SHTC3 温湿度 + 电池 + 按键 |
| `esp32-s3-rlcd-42-speaker.yaml` | 传感器面板 + ES8311 扬声器铃声 |
| `esp32-s3-rlcd-42-wake-word.yaml` | "Hey Jarvis" 离线唤醒词 |

### 5.5 Arduino 示例（10 个 .ino）

路径：`02_Example/Arduino/`（与 ESP-IDF 示例一一对应，不含 FactoryProgram）

---

## 6. 社区固件项目（18 个）

### 6.1 汇总矩阵

| 项目 | 类型 | 语言 | 成熟度 | 适用场景 |
|------|------|------|--------|----------|
| **kylehase/ESPHome-ST7305-RLCD** | ESPHome 驱动 | C++/Python | 生产级 | Home Assistant 集成 |
| **esphome-devices config** | ESPHome 配置 | YAML | 可用 | 快速 ESPHome 启动 |
| **zjyl1994/MicroPython** | 完整固件 | C/Python | 高级 | LVGL + MicroPython 原生 |
| **nunombispo/weather** | MicroPython 应用 | Python | 完善 | 天气站 |
| **peterhinch/micro-gui** | MicroPython GUI | Python | 成熟 | 交互式 UI |
| **peterhinch/nano-gui** | MicroPython GUI | Python | 成熟 | 静态仪表盘 |
| **franklighter/RLCD-Driver** | MicroPython 驱动 | Python | 基础 | 简单 framebuf |
| **ahdrage/Dashy** | Arduino 应用 | C++ | 有文档 | 家居仪表盘（Netatmo） |
| **wudingjian/waveshare** | XiaoZhi 分支 | C++ | 活跃 | AI助手 + 天气 |
| **la-lo-go/trmnl** | ESP-IDF 应用 | C | 架构良好 | 云管理电子墨水屏 |
| **Zephyr RTOS** | 替代 OS | C | 官方(新) | Zephyr 生态 |
| **esp-claw** | AI Agent 框架 | C++ | 官方(测试) | 边缘 AI Agent |
| **Matt-DESTROYER/st7305** | Rust 驱动 | Rust | 早期 | Rust/no_std |
| **st7305-driver** | Rust 驱动 | Rust | 早期 | Rust + esp-hal |
| **u8g2** | 图形库 | C | 久经考验 | Arduino 单色图形 |
| **OneBitDisplay** | 图形库 | C | 成熟 | u8g2 替代 |
| **elulis/micropython_ST7302** | MicroPython 驱动 | Python | 基础 | ST7302/7305 |

---

## 7. WiFi 兼容性分析

### 7.1 用户 WiFi 环境

| 项目 | 值 |
|------|------|
| SSID | `rickqi11` |
| 密码 | `18620907850` |
| 认证 | WPA2/WPA3 个人 |
| 当前频段 | 5 GHz（信道 157） |
| 信号强度 | 78% |

### 7.2 兼容性问题（已解决）

- **ESP32-S3 仅支持 2.4 GHz**，用户路由器 rickqi11 为双频（2.4GHz + 5GHz）
- ESP32-S3 成功连接到 2.4GHz 频段：信道 2, BW20, RSSI -58 dBm
- **状态：已验证连接成功**，分配 IP 192.168.50.36

### 7.3 源码修改记录

**① FactoryProgram**（`02_Example/ESP-IDF/10_FactoryProgram/components/app_bsp/esp_wifi_bsp.c`）：

```diff
- .ssid = "PDCN",
- .password = "1234567890",
+ .ssid = "rickqi11",
+ .password = "18620907850",
```

**② WIFI_STA**（`02_Example/ESP-IDF/02_WIFI_STA/components/esp_wifi_bsp/esp_wifi_bsp.c`）：

```diff
- .ssid     = "K2P",
- .password = "1234567890",
+ .ssid     = "rickqi11",
+ .password = "18620907850",
```

**③ WIFI_STA 显示定制**（新增组件，详见 SETUP_LOG.md 阶段十）：

| 文件 | 操作 |
|------|------|
| `main/main.cpp` | 重写：WiFi 状态 UI + ST7305 显示 + 后台 RSSI 刷新 |
| `components/port_bsp/` | 新增：display_bsp.cpp/h（ST7305 LCD 驱动） |
| `components/app_bsp/` | 新增：lvgl_bsp.cpp/h（LVGL 端口层） |
| `components/esp_wifi_bsp/esp_wifi_bsp.h` | 修改：加 `extern "C"` |
| `main/idf_component.yml` | 新增：lvgl/lvgl ^8.4.0 |
| `sdkconfig.defaults` | 修改：添加 LVGL 配置项 |

---

## 8. 烧录操作记录

### 8.1 擦除 Flash

```bash
python -m esptool --port COM4 --baud 921600 erase-flash
# 结果：Flash memory erased successfully in 5.3 seconds.
```

### 8.2 烧录出厂固件

```bash
python -m esptool --port COM4 --baud 921600 write-flash 0x0 "03_Firmware/01_Factory_V1.bin"
# 结果：Wrote 4548144 bytes (2963601 compressed) in 25.3 seconds (1440.9 kbit/s)
# 校验：Hash of data verified.
```

### 8.3 固件验证

```bash
python -m esptool --port COM4 --baud 921600 read-flash 0x10020 0x100 verify_appdesc.bin
# 解析 esp_app_desc_t:
#   project:  03_Fac
#   version:  1
#   date:     Jan 20 2026 18:13:54
#   idf_ver:  v5.5.2
```

---

## 9. GPIO 引脚映射

来源：`XiaoZhiCode_V2.1.0/main/boards/waveshare-s3-rlcd-4.2/config.h`

| GPIO | 功能 | 说明 |
|:----:|------|------|
| GPIO0 | BOOT 按键 | 低电平有效，兼做启动模式选择 |
| GPIO4 | 电池 ADC | 电池电压检测 |
| GPIO5 | Display DC | 显示屏数据/命令选择 |
| GPIO6 | Display TE | 显示屏 Tearing Effect 信号 |
| GPIO8 | I²S DOUT | 音频输出（扬声器） |
| GPIO9 | I²S BCLK | I²S 位时钟 |
| GPIO10 | I²S DIN | 音频输入（麦克风） |
| GPIO11 | SPI CLK | 显示屏 SPI 时钟 |
| GPIO12 | SPI MOSI | 显示屏 SPI 数据 |
| GPIO13 | I²C SDA | I²C 数据 |
| GPIO14 | I²C SCL | I²C 时钟 |
| GPIO16 | I²S MCLK | 音频主时钟 |
| GPIO18 | KEY 按键 | 用户按键 |
| GPIO40 | Display CS | 显示屏片选 |
| GPIO41 | Display RST | 显示屏复位 |
| GPIO45 | I²S LRCLK | I²S 左右声道时钟 |
| GPIO46 | 功放使能 | 扬声器放大器开关 |

### I²C 设备地址

| 设备 | 地址 |
|------|:----:|
| ES8311 | `0x18` |
| ES7210 | `0x40` |
| PCF85063 | 参考数据手册 |
| SHTC3 | 参考数据手册 |

---

## 10. 已完成事项

| # | 任务 | 状态 | 结果 |
|---|------|------|------|
| 1 | 确认路由器 2.4GHz 可用 | ✅ 已完成 | rickqi11 双频，2.4GHz 信道 2 |
| 2 | 安装 ESP-IDF v5.5.x | ✅ 已完成 | v5.5.2，工具链 esp-14.2.0 |
| 3 | 编译修改后的 FactoryProgram | ✅ 已完成 | 03_Fac.bin 4.27MB，47% 分区 |
| 4 | 烧录 FactoryProgram 并验证硬件 | ✅ 已完成 | 全部硬件正常（LCD/SHTC3/SD/音频/RTC/电池） |
| 5 | 编译 WIFI_STA 并验证 WiFi 连接 | ✅ 已完成 | IP 192.168.50.36, RSSI -58 dBm |
| 6 | WIFI_STA 显示功能定制开发 | ✅ 已完成 | LVGL UI + ST7305 显示，每 2 秒刷新 |
| 7 | （可选）烧录小智 AI 固件体验语音助手 | ⏳ 待定 | 可随时执行 |

---

*报告生成工具：OpenCode (Sisyphus) | esptool v5.3.1 | ESP-IDF v5.5.2*
