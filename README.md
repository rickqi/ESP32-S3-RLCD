# ESP32-S3-RLCD-4.2 安装使用手册

> 基于微雪电子 [ESP32-S3-RLCD-4.2](https://www.waveshare.net/shop/ESP32-S3-RLCD-4.2.htm) 全反射屏 AIoT 开发板

---

## 目录

- [1. 产品概述](#1-产品概述)
- [2. 硬件规格](#2-硬件规格)
- [3. 引脚映射](#3-引脚映射)
- [4. 目录结构](#4-目录结构)
- [5. 快速开始：烧录预编译固件](#5-快速开始烧录预编译固件)
- [6. Arduino 开发](#6-arduino-开发)
- [7. ESP-IDF 开发](#7-esp-idf-开发)
- [8. ESPHome 开发（Home Assistant）](#8-esphome-开发home-assistant)
- [9. 小智 AI 聊天机器人](#9-小智-ai-聊天机器人)
- [10. 示例程序说明](#10-示例程序说明)
- [11. 资料清单](#11-资料清单)
- [12. 常见问题](#12-常见问题)
- [13. Git 仓库与资源获取](#13-git-仓库与资源获取)

---

## 1. 产品概述

ESP32-S3-RLCD-4.2 是一款基于 ESP32-S3-WROOM-1-N16R8 的全反射屏 AIoT 开发板，具有以下特点：

- **4.2 英寸全反射 LCD**（ST7305 驱动，300×400 分辨率），无需背光，阳光下可读，极低功耗
- **音频模块**：ES8311 编解码器 + ES7210 双麦克风阵列，支持语音交互
- **板载传感器**：SHTC3 温湿度传感器、PCF85063 实时时钟（RTC）
- **丰富接口**：TF 卡槽、USB Type-C、18650 电池座（带充放电管理）
- **无线连接**：2.4 GHz Wi-Fi + Bluetooth 5 LE
- **AI 应用**：支持小智 AI 语音助手、ESPHome 智能家居集成

适用场景：智能摆件、电子日历、AI 语音助手、物联网传感器节点。

---

## 2. 硬件规格

| 项目 | 参数 |
|------|------|
| 主控芯片 | ESP32-S3-WROOM-1-N16R8 |
| CPU | 双核 Xtensa LX7 @ 240 MHz |
| Flash | 16 MB |
| PSRAM | 8 MB (octal) |
| 显示屏 | 4.2" 反射式 LCD (ST7305)，300×400 像素，1bit 灰度 |
| 音频编解码 | ES8311（扬声器输出） |
| 麦克风 | ES7210 双通道麦克风阵列 |
| RTC | PCF85063TLH |
| 温湿度传感器 | SHTC3 |
| 无线 | 2.4 GHz Wi-Fi (802.11 b/g/n)、Bluetooth 5 (LE) |
| 供电 | USB Type-C 5V / 18650 锂电池（3.7V） |
| 尺寸 | 106.5 × 84 mm |

---

## 3. 引脚映射

### 3.1 完整 GPIO 分配表

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
| GPIO13 | I²C SDA | I²C 数据（ES8311/ES7210/PCF85063/SHTC3） |
| GPIO14 | I²C SCL | I²C 时钟 |
| GPIO16 | I²S MCLK | 音频主时钟 |
| GPIO18 | KEY 按键 | 用户按键，低电平有效 |
| GPIO40 | Display CS | 显示屏片选 |
| GPIO41 | Display RST | 显示屏复位 |
| GPIO45 | I²S LRCLK | I²S 左右声道时钟 |
| GPIO46 | 功放使能 | 扬声器放大器开关 |

### 3.2 I²C 设备地址

| 设备 | 地址 | 说明 |
|------|:----:|------|
| ES8311 | `0x18` | 音频编解码器 |
| ES7210 | `0x40` | 麦克风阵列 |
| PCF85063 | — | RTC（参考数据手册） |
| SHTC3 | — | 温湿度传感器（参考数据手册） |

### 3.3 显示屏参数

| 参数 | 值 |
|------|------|
| 驱动 IC | ST7305 |
| 接口 | SPI（4 线） |
| SPI 时钟 | 10 MHz |
| 分辨率 | 400 × 300（横屏）/ 300 × 400（竖屏） |
| 色深 | 1bit 灰度（黑白） |
| 缓冲区 | 分配于 PSRAM |

---

## 4. 目录结构

```
ESP32-S3-RLCD/
├── docs/                          # 技术资料
│   ├── hardware/                  # 原理图、3D 结构文件
│   │   ├── ESP32-S3-RLCD-4.2-schematic.pdf
│   │   └── ESP32-S3-RLCD-4.2-3dFile.rar
│   ├── datasheets/                # 芯片数据手册
│   │   ├── ESP32-S3_datasheet_cn.pdf
│   │   ├── ESP32-S3_datasheet_en.pdf
│   │   ├── ESP32-S3_technical_reference_manual_cn.pdf
│   │   ├── ESP32-S3_technical_reference_manual_en.pdf
│   │   ├── ST7305_datasheet.pdf
│   │   ├── ES8311_datasheet.pdf
│   │   ├── PCF85063_datasheet.pdf
│   │   └── SHTC3_datasheet.pdf
│   ├── examples/                  # 示例程序
│   │   ├── ESP32-S3-RLCD-4.2-Demo.zip    # 打包下载的完整示例
│   │   └── ESP32-S3-RLCD-4.2-GitHub/     # Git 仓库（完整源码）
│   │       ├── 01_Arduino_Libraries/     # Arduino 第三方依赖库
│   │       │   ├── lvgl8/                # LVGL v8（用于 v8 示例）
│   │       │   ├── lvgl9/                # LVGL v9（用于 v9 示例）
│   │       │   ├── SensorLib/            # 传感器驱动库
│   │       │   └── U8g2/                 # 单色显示屏库
│   │       │   └── ReadMe.txt            # 库安装说明
│   │       │
│   │       ├── 02_Example/               # 示例代码
│   │       │   ├── Arduino/              # Arduino IDE 示例（10 个）
│   │       │   ├── ESP-IDF/              # ESP-IDF 示例（11 个）
│   │       │   ├── ESPHome/              # ESPHome YAML 配置（3 个）
│   │       │   └── XiaoZhi/              # 小智 AI 源码
│   │       │       └── XiaoZhiCode_V2.1.0/
│   │       │
│   │       └── 03_Firmware/              # 预编译固件
│   │           ├── 01_Factory_V1.bin     # 出厂测试固件
│   │           └── 02_XiaoZhi_V2.1.0.bin # 小智 AI 固件
│   │
│   └── community/               # 社区资源
│       ├── Bilibili-AI-Assistant-ESP32-S3.mp4  # 演示视频
│       └── xiaozhi-esp32.zip    # 社区 xiaozhi-esp32 仓库
│
└── README.md                     # 本文件
```

---

## 5. 快速开始：烧录预编译固件

### 5.1 所需工具

- USB Type-C 数据线
- [Flash 下载工具](https://www.espressif.com/en/support/download/other-tools)（ESP Flash Download Tool）或 [esptool.py](https://github.com/espressif/esptool)

### 5.2 使用 esptool.py 烧录

```bash
# 安装 esptool
pip install esptool

# 擦除 Flash（首次烧录建议执行）
esptool.py --port COM3 erase_flash

# 烧录出厂固件
esptool.py --port COM3 --baud 921600 write_flash 0x0 docs/examples/ESP32-S3-RLCD-4.2-GitHub/03_Firmware/01_Factory_V1.bin

# 或烧录小智 AI 固件
esptool.py --port COM3 --baud 921600 write_flash 0x0 docs/examples/ESP32-S3-RLCD-4.2-GitHub/03_Firmware/02_XiaoZhi_V2.1.0.bin
```

> **注意**：将 `COM3` 替换为你的实际端口号。Windows 设备管理器中查看端口编号。

### 5.3 使用 ESP Flash Download Tool

1. 下载并打开 ESP Flash Download Tool
2. 选择芯片类型：**ESP32-S3**
3. 选择工作模式：**Develop**
4. 加载固件 `.bin` 文件，地址设为 `0x0`
5. 选择对应 COM 端口，波特率 921600
6. 点击 **ERASE** 擦除 Flash
7. 点击 **START** 开始烧录

---

## 6. Arduino 开发

### 6.1 环境准备

#### 6.1.1 安装 Arduino IDE

下载并安装 [Arduino IDE 2.x](https://www.arduino.cc/en/software)。

#### 6.1.2 添加 ESP32 板卡支持

1. 打开 **文件 → 首选项**
2. 在「附加开发板管理器 URL」中添加：
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. 打开 **工具 → 开发板 → 开发板管理器**
4. 搜索 `esp32`，安装 **esp32 by Espressif Systems**（版本 ≥ 3.0.0）

#### 6.1.3 开发板配置

打开 **工具** 菜单，按以下参数设置：

| 设置项 | 值 |
|--------|-----|
| Board | **ESP32S3 Dev Module** |
| USB Mode | **USB-OTG (TinyUSB)** |
| USB CDC On Boot | **Enabled** |
| Upload Mode | **UART0 / Hardware CDC** |
| Flash Size | **16MB (128Mb)** |
| Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** 或自定义 |
| PSRAM | **OPI PSRAM 8MB** |
| Upload Speed | **921600** |
| Port | 选择你的 COM 端口 |

#### 6.1.4 安装第三方库

将 `01_Arduino_Libraries/` 下的库复制到 Arduino 库目录：

- **Windows**: `文档/Arduino/libraries/`
- **Mac**: `~/Documents/Arduino/libraries/`
- **Linux**: `~/Arduino/libraries/`

| 库 | 用途 | 适用示例 |
|----|------|----------|
| `SensorLib` | I²C 传感器驱动（PCF85063、SHTC3） | 04、05 |
| `U8g2` | 单色图形库 | 10_U8G2_Test |
| `lvgl8` | LVGL v8 GUI 框架 | 08_LVGL_V8_Test |
| `lvgl9` | LVGL v9 GUI 框架 | 09_LVGL_V9_Test |

> **重要**：LVGL v8 和 v9 **互斥**，同一时间只能安装一个版本。使用 v8 示例时装入 `lvgl8`，使用 v9 示例时装入 `lvgl9`。

### 6.2 编译上传示例

1. 用 Arduino IDE 打开 `02_Example/Arduino/` 下的示例 `.ino` 文件
2. 确认开发板配置正确
3. 点击 **上传** 按钮（→）
4. 等待编译和烧录完成

---

## 7. ESP-IDF 开发

### 7.1 环境安装

#### 7.1.1 安装 ESP-IDF

```bash
# 克隆 ESP-IDF（建议 v5.2 或以上）
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3   # Linux/Mac
# 或 Windows 下运行 install.bat
```

#### 7.1.2 激活环境

```bash
# 每次打开新终端时执行
source ~/esp/esp-idf/export.sh    # Linux/Mac
# 或 Windows: %userprofile%\esp\esp-idf\export.bat
```

### 7.2 编译烧录

```bash
cd 02_Example/ESP-IDF/08_LVGL_V8_Test

# 设置目标芯片
idf.py set-target esp32s3

# 配置（可选）
idf.py menuconfig

# 编译
idf.py build

# 烧录（替换 COM3 为实际端口）
idf.py -p COM3 flash

# 串口监视
idf.py -p COM3 monitor
```

> 按 `Ctrl + ]` 退出串口监视器。

---

## 8. ESPHome 开发（Home Assistant）

### 8.1 适用场景

通过 YAML 配置即可将设备接入 [Home Assistant](https://www.home-assistant.io/) 智能家居系统，无需编写 C/C++ 代码。

### 8.2 快速开始

1. 在 Home Assistant 中安装 [ESPHome add-on](https://esphome.io/guides/getting_started_hassio.html)
2. 创建新设备，点击 **EDIT**
3. 粘贴 `02_Example/ESPHome/examples/` 下的 YAML 配置
4. 创建 `secrets.yaml` 并填入 Wi-Fi 信息：
   ```yaml
   wifi_ssid: "你的WiFi名称"
   wifi_password: "你的WiFi密码"
   ```
5. **SAVE → INSTALL**，首次通过 USB 烧录，后续可 OTA 升级

### 8.3 可用配置

| 配置文件 | 功能说明 |
|----------|----------|
| `esp32-s3-rlcd-42-sensor.yaml` | 屏幕显示 + SHTC3 温湿度 + 电池电量 + 按键 |
| `esp32-s3-rlcd-42-speaker.yaml` | 传感器面板 + ES8311 扬声器 RTTTL 铃声测试 |
| `esp32-s3-rlcd-42-wake-word.yaml` | 设备端 "Hey Jarvis" 离线唤醒词检测 |

> **依赖**：需安装 [ESPHome-ST7305-RLCD 外部组件](https://github.com/kylehase/ESPHome-ST7305-RLCD)。

---

## 9. 小智 AI 聊天机器人

### 9.1 简介

[小智 AI](https://github.com/78/xiaozhi-esp32) 是一个开源的 MCP 协议语音聊天机器人，支持接入 Qwen、DeepSeek 等大模型，实现：

- 流式 ASR（语音转文字）+ LLM + TTS 语音交互
- 离线语音唤醒（ESP-SR）
- 声纹识别
- 设备端 MCP 控制（音量、灯光、GPIO 等）
- 云端 MCP 扩展（智能家居控制、知识搜索等）
- 表情显示、电量管理、多语言支持

### 9.2 快速烧录（预编译固件）

```bash
esptool.py --port COM3 erase_flash
esptool.py --port COM3 --baud 921600 write_flash 0x0 03_Firmware/02_XiaoZhi_V2.1.0.bin
```

### 9.3 从源码编译

```bash
cd 02_Example/XiaoZhi/XiaoZhiCode_V2.1.0

# 设置目标芯片
idf.py set-target esp32s3

# 配置板卡
idf.py menuconfig
# → XiaoZhi IoT → Board Type → 选择 "waveshare-s3-rlcd-4.2"

# 编译
idf.py build

# 烧录
idf.py -p COM3 flash monitor
```

### 9.4 首次配网

1. 烧录完成后，屏幕显示配网二维码
2. 手机扫描二维码或访问 [小智配网页面](https://xiaozhi.me)
3. 填入 Wi-Fi 信息，绑定设备
4. 配网成功后即可语音交互

> 详细教程：[手工打造你的 AI 女友，新手入门教程](https://www.bilibili.com/video/BV1XnmFYLEJN/)

---

## 10. 示例程序说明

### 10.1 Arduino / ESP-IDF 示例

| 编号 | 示例 | 功能说明 |
|:----:|------|----------|
| 01 | WIFI_AP | 创建 Wi-Fi 热点（AP 模式） |
| 02 | WIFI_STA | 连接 Wi-Fi 路由器（STA 模式） |
| 03 | ADC_Test | 电池电压 ADC 采集 |
| 04 | I2C_PCF85063 | RTC 时间读写 |
| 05 | I2C_SHTC3 | 温湿度传感器数据读取 |
| 06 | SD_Card | TF 卡文件读写 |
| 07 | Audio_Test | ES8311 扬声器播放 + ES7210 麦克风录制 |
| 08 | LVGL_V8_Test | LVGL v8 GUI 界面演示 |
| 09 | LVGL_V9_Test | LVGL v9 GUI 界面演示 |
| 10 | U8G2_Test | U8g2 图形库演示 |
| 11 | FactoryProgram | 出厂综合测试程序（仅 ESP-IDF） |

### 10.2 显示屏驱动说明

反射式 LCD 采用 ST7305 驱动 IC，关键特性：

- **1bit 灰度显示**：每个像素仅黑白两色（无灰阶）
- **SPI 接口**：4 线 SPI，时钟 10 MHz
- **缓冲区**：全屏缓冲区 15000 字节（400×300/8），分配于 PSRAM
- **三种刷新算法**：
  - `AlgorithmOptimization = 1`：原始算法
  - `AlgorithmOptimization = 2`：位移优化
  - `AlgorithmOptimization = 3`：查表法（默认，性能最优）
- **支持横屏/竖屏**：通过 LUT（Look-Up Table）自动转换坐标

---

## 11. 资料清单

### 11.1 硬件资料

| 文件 | 路径 |
|------|------|
| 原理图 | `docs/hardware/ESP32-S3-RLCD-4.2-schematic.pdf` |
| 3D 结构文件 | `docs/hardware/ESP32-S3-RLCD-4.2-3dFile.rar` |

### 11.2 技术手册

| 文件 | 路径 |
|------|------|
| ESP32-S3 技术规格书（中文） | `docs/datasheets/ESP32-S3_datasheet_cn.pdf` |
| ESP32-S3 技术规格书（英文） | `docs/datasheets/ESP32-S3_datasheet_en.pdf` |
| ESP32-S3 技术参考手册（中文） | `docs/datasheets/ESP32-S3_technical_reference_manual_cn.pdf` |
| ESP32-S3 技术参考手册（英文） | `docs/datasheets/ESP32-S3_technical_reference_manual_en.pdf` |
| ST7305 数据手册 | `docs/datasheets/ST7305_datasheet.pdf` |
| ES8311 数据手册 | `docs/datasheets/ES8311_datasheet.pdf` |
| PCF85063 数据手册 | `docs/datasheets/PCF85063_datasheet.pdf` |
| SHTC3 数据手册 | `docs/datasheets/SHTC3_datasheet.pdf` |

### 11.3 在线资源

| 资源 | 链接 |
|------|------|
| 产品文档 | <https://docs.waveshare.net/ESP32-S3-RLCD-4.2> |
| GitHub 仓库 | <https://github.com/waveshareteam/ESP32-S3-RLCD-4.2> |
| 购买链接 | <https://www.waveshare.net/shop/ESP32-S3-RLCD-4.2.htm> |
| 小智 AI 项目 | <https://github.com/78/xiaozhi-esp32> |
| ESPHome ST7305 组件 | <https://github.com/kylehase/ESPHome-ST7305-RLCD> |
| ESPHome 文档 | <https://esphome.io/> |

---

## 12. 常见问题

### Q: 屏幕没有背光，看起来很暗？

这是反射式 LCD 的正常表现。该屏幕**不需要背光**，依靠环境光反射显示内容。在光线充足的环境下效果最佳，阳光下也可清晰阅读。

### Q: Arduino 编译报错找不到 LVGL 库？

LVGL v8 和 v9 是两个互斥的版本。使用 LVGL v8 示例时，只能安装 `lvgl8` 目录下的库文件；使用 v9 示例时，只能安装 `lvgl9` 目录下的库文件。请确认库已正确复制到 Arduino 的 `libraries/` 目录。

### Q: 烧录后串口无输出？

检查 Arduino IDE 中 **USB CDC On Boot** 是否设为 **Enabled**。ESP-IDF 下检查 `menuconfig → Component config → ESP System Settings → Channel for console output` 是否设为 USB。

### Q: 如何选择分区表？

- **Arduino**：推荐 `16M Flash (3MB APP/9.9MB FATFS)`
- **ESP-IDF（小智 AI）**：使用 XiaoZhi 项目自带的分区表，参考 `partitions/v2/` 目录

### Q: 小智 AI 无法配网？

1. 确认 Wi-Fi 为 2.4GHz（不支持 5GHz）
2. 检查设备是否进入配网模式（屏幕显示二维码）
3. 如之前配过网，长按 BOOT 键 5 秒以上重置

### Q: 电池不充电 / 电池电量显示不准确？

1. 确认使用 18650 锂电池（3.7V，不带保护板也可）
2. USB 接入后充电指示灯应亮起
3. 电量 ADC 读数需要校准，参考 `03_ADC_Test` 示例

### Q: LVGL 刷新缓慢？

反射式 LCD 为 1bit 灰度屏，每帧需逐像素转换。确保 `AlgorithmOptimization` 设为 `3`（查表法），并将显示缓冲区分配在 PSRAM 中（已在代码中默认配置）。

---

## 13. Git 仓库与资源获取

本仓库仅提交**源码和文档**（约 2 MB），所有二进制资料、编译产物和第三方依赖通过以下方式获取。

### 13.1 克隆后初始化环境

```bash
# 1. 克隆仓库
git clone <repo-url> ESP32-S3-RLCD
cd ESP32-S3-RLCD

# 2. 获取官方示例源码（含第三方库 + 预编译固件）
#    方式 A：直接克隆微雪 GitHub 仓库
git clone https://github.com/waveshareteam/ESP32-S3-RLCD-4.2.git docs/examples/ESP32-S3-RLCD-4.2-GitHub

#    方式 B：下载打包 ZIP（含 Arduino 库，无需单独安装）
#    https://github.com/waveshareteam/ESP32-S3-RLCD-4.2/archive/refs/heads/main.zip
#    解压到 docs/examples/ESP32-S3-RLCD-4.2-GitHub/
```

### 13.2 被排除的内容及获取方式

| 排除项 | 路径 | 大小 | 获取方式 |
|--------|------|------|----------|
| **编译产物** | `**/build/` | ~720 MB | `idf.py build` 自动生成 |
| **托管依赖** | `**/managed_components/` | ~220 MB | `idf.py set-target esp32s3` 自动下载 |
| **sdkconfig** | `**/sdkconfig` | ~0.8 MB | `idf.py menuconfig` 或从 `sdkconfig.defaults` 生成 |
| **Demo ZIP** | `docs/examples/ESP32-S3-RLCD-4.2-Demo.zip` | 177 MB | [GitHub Release](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2/releases) 下载 |
| **Arduino 库** | `01_Arduino_Libraries/` | 421 MB | 上方 13.1 克隆或 ZIP 下载 |
| **预编译固件** | `03_Firmware/*.bin` | 15 MB | [GitHub Release](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2/releases) 下载 |
| **芯片手册** | `docs/datasheets/` | 40 MB | [微雪文档站](https://docs.waveshare.net/ESP32-S3-RLCD-4.2) → "下载" 栏目 |
| **原理图** | `docs/hardware/` | 5.5 MB | 同上 |
| **社区视频** | `docs/community/` | 40 MB | [Bilibili BV1PsfhBCEqA](https://www.bilibili.com/video/BV1PsfhBCEqA) |
| **XiaoZhi 源码** | `XiaoZhiCode_V2.1.0/` | 5.4 MB | `git clone https://github.com/78/xiaozhi-esp32.git` |
| **截图输出** | `screenshot_*.png/pbm` | ~18 KB | `python screenshot.py COM4` 运行生成 |
| **OpenCode 会话** | `.omo/` | — | OpenCode IDE 自动生成 |

### 13.3 从源码编译固件

```bash
# ESP-IDF 环境（需先安装 v5.5.x）
cd docs/examples/ESP32-S3-RLCD-4.2-GitHub/02_Example/ESP-IDF/02_WIFI_STA
idf.py set-target esp32s3   # 自动下载 managed_components（LVGL 等）
idf.py build                # 生成 build/ 目录 + .bin 固件
idf.py -p COM4 flash monitor
```

> **WiFi STA 定制固件**已包含在仓库源码中（`02_WIFI_STA/main/main.cpp`），包含 ST7305 显示驱动、LVGL UI 和串口截屏功能。WiFi 凭据在 `components/esp_wifi_bsp/esp_wifi_bsp.c` 中修改。

### 13.4 截屏工具

```bash
# 安装依赖
pip install pyserial Pillow

# 自动模式：复位设备后自动截图（WiFi 连接成功后触发）
python screenshot.py COM4

# 按键模式：监听串口，按设备 BOOT 键触发
python screenshot.py COM4 --listen
```

### 13.5 目录结构（Git 仓库）

```
ESP32-S3-RLCD/
├── README.md                         # 安装使用手册
├── FIRMWARE_REPORT.md                # 固件分析报告
├── SETUP_LOG.md                      # 安装调试日志
├── screenshot.py                     # 屏幕截图工具
├── .gitignore
└── docs/examples/ESP32-S3-RLCD-4.2-GitHub/
    └── 02_Example/ESP-IDF/
        ├── 02_WIFI_STA/              # ← 我们的定制固件
        │   ├── main/
        │   │   ├── main.cpp          # WiFi + LVGL + 截屏
        │   │   ├── CMakeLists.txt
        │   │   └── idf_component.yml
        │   ├── components/
        │   │   ├── port_bsp/         # ST7305 LCD 驱动
        │   │   ├── app_bsp/          # LVGL 端口层
        │   │   └── esp_wifi_bsp/     # WiFi 初始化
        │   └── sdkconfig.defaults
        ├── 01~11_*/                  # 原始示例源码（未修改）
        └── 10_FactoryProgram/
            └── components/app_bsp/
                └── esp_wifi_bsp.c    # ← WiFi 凭据已修改
```

---

*本手册基于 ESP32-S3-RLCD-4.2 仓库及微雪官方文档整理。如有疑问请参考[产品文档](https://docs.waveshare.net/ESP32-S3-RLCD-4.2)或提交 [GitHub Issue](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2/issues)。*
